#pragma once

#include <Core/Basics.h>
#include <Core/ResourceManager/ResourceBase.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/UniquePtr.h>

enum class ezResourceManagerEventType
{
  ManagerShuttingDown,
  ResourceCategoryChanged,
  ReloadAllResources,     ///< Set by ReloadAllResources() if any resource got unloaded (not yet reloaded)
};

struct ezResourceManagerEvent
{
  ezResourceManagerEventType m_EventType;
  const ezResourceCategory* m_pCategory;
};

/// \brief [internal] Worker thread/task for loading resources from disk.
class EZ_CORE_DLL ezResourceManagerWorkerDiskRead : public ezTask
{
private:
  friend class ezResourceManager;

  ezResourceManagerWorkerDiskRead(){};

  static void DoWork(bool bCalledExternally);

  virtual void Execute() override;
};

/// \brief [internal] Worker thread/task for loading on the main thread.
class EZ_CORE_DLL ezResourceManagerWorkerMainThread : public ezTask
{
public:
  ezResourceLoadData m_LoaderData;
  ezResource* m_pResourceToLoad;
  ezResourceTypeLoader* m_pLoader;
  // this is only used to clean up a custom loader at the right time, if one is used
  // m_pLoader is always set, no need to go through m_pCustomLoader
  ezUniquePtr<ezResourceTypeLoader> m_pCustomLoader;

private:
  friend class ezResourceManager;
  ezResourceManagerWorkerMainThread(){};

  virtual void Execute() override;
};

class EZ_CORE_DLL ezResourceManager
{
public:
  static ezEvent<const ezResourceEvent&> s_ResourceEvents;
  static ezEvent<const ezResourceManagerEvent&> s_ManagerEvents;

  /// \brief Registers which resource type to use to load an asset with the given type name
  static void RegisterResourceForAssetType(const char* szAssetTypeName, const ezRTTI* pResourceType);

  /// \brief Returns the resource type that was registered to handle the given asset type for loading. nullptr if no resource type was
  /// registered for this asset type.
  static const ezRTTI* FindResourceForAssetType(const char* szAssetTypeName);

  /// \brief Same as LoadResource(), but instead of a template argument, the resource type to use is given as ezRTTI info. Returns a
  /// typeless handle due to the missing template argument.
  static ezTypelessResourceHandle LoadResourceByType(const ezRTTI* pResourceType, const char* szResourceID);

  /// \brief Returns a handle to the requested resource. szResourceID must uniquely identify the resource, different spellings will result
  /// in different resources.
  ///
  /// After the call to this function the resource definitely exists in memory. Upon access through BeginAcquireResource the resource will
  /// be loaded. If it is not possible to load the resource it will change to a 'missing' state. If the code accessing the resource cannot
  /// handle that case, the application will 'terminate' (that means crash).
  template <typename ResourceType>
  static ezTypedResourceHandle<ResourceType> LoadResource(const char* szResourceID);

  /// \brief Same as LoadResource(), but additionally allows to set a priority on the resource and a custom fallback resource for this
  /// instance.
  template <typename ResourceType>
  static ezTypedResourceHandle<ResourceType> LoadResource(const char* szResourceID, ezResourcePriority Priority,
                                                          ezTypedResourceHandle<ResourceType> hFallbackResource);

  /// \brief Creates a resource from code.
  ///
  /// \param szResourceID The unique ID by which the resource is identified. E.g. in GetExistingResource()
  /// \param descriptor A type specific descriptor that holds all the information to create the resource.
  /// \param szResourceDescription An optional description that might help during debugging. Often a human readable name or path is stored
  /// here, to make it easier to identify this resource.
  template <typename ResourceType, typename DescriptorType>
  static ezTypedResourceHandle<ResourceType> CreateResource(const char* szResourceID, DescriptorType&& descriptor,
                                                            const char* szResourceDescription = nullptr);

  /// \brief Returns a handle to the resource with the given ID. If the resource does not exist, the handle is invalid.
  ///
  /// Use this if a resource needs to be created procedurally (with CreateResource()), but it might already have been created.
  /// If the returned handle is invalid, then just go through the resource creation step.
  template <typename ResourceType>
  static ezTypedResourceHandle<ResourceType> GetExistingResource(const char* szResourceID);

  /// \brief Acquires a resource pointer from a handle. Prefer to use ezResourceLock, which wraps BeginAcquireResource / EndAcquireResource
  ///
  /// \param hResource The resource to acquire
  /// \param mode The desired way to acquire the resource. See ezResourceAcquireMode for details.
  /// \param hFallbackResource A custom fallback resource that should be returned if hResource is not yet available. Allows to use domain
  /// specific knowledge to get a better fallback. \param Priority Allows to adjust the priority of the resource. This will affect how fast
  /// the resource is loaded, when it was not yet available.
  template <typename ResourceType>
  static ResourceType*
  BeginAcquireResource(const ezTypedResourceHandle<ResourceType>& hResource,
                       ezResourceAcquireMode mode = ezResourceAcquireMode::AllowFallback,
                       const ezTypedResourceHandle<ResourceType>& hFallbackResource = ezTypedResourceHandle<ResourceType>(),
                       ezResourcePriority Priority = ezResourcePriority::Unchanged, ezResourceAcquireResult* out_AcquireResult = nullptr);

  template <typename ResourceType>
  static void EndAcquireResource(ResourceType* pResource);

  /// \brief Sets the resource loader to use for the given resource type.
  template <typename ResourceType>
  static void SetResourceTypeLoader(ezResourceTypeLoader* pCreator);

  /// \brief Sets the resource loader to use when no type specific resource loader is available.
  static void SetDefaultResourceLoader(ezResourceTypeLoader* pDefaultLoader);

  /// \brief Returns the resource loader to use when no type specific resource loader is available.
  static ezResourceTypeLoader* GetDefaultResourceLoader() { return m_pDefaultResourceLoader; }

  /// \brief Triggers loading of the given resource. tShouldBeAvailableIn specifies how long the resource is not yet needed, thus allowing
  /// other resources to be loaded first.
  static void PreloadResource(const ezTypelessResourceHandle& hResource, ezTime tShouldBeAvailableIn);

  /// \brief Deallocates all resources whose refcount has reached 0. Returns the number of deleted resources.
  static ezUInt32 FreeUnusedResources(bool bFreeAllUnused);

  /// \brief Removes the 'PreventFileReload' flag and forces a reload on the resource.
  template <typename ResourceType>
  static void RestoreResource(const ezTypedResourceHandle<ResourceType>& hResource);

  template <typename ResourceType>
  static bool ReloadResource(const ezTypedResourceHandle<ResourceType>& hResource, bool bForce);

  /// \brief Goes through all resources of the given type and makes sure they are reloaded, if they have changed. If bForce is true,
  /// resources are updated, even if there is no indication that they have changed.
  template <typename ResourceType>
  static ezUInt32 ReloadResourcesOfType(bool bForce);

  /// \brief Goes through all resources of the given type and makes sure they are reloaded, if they have changed. If bForce is true,
  /// resources are updated, even if there is no indication that they have changed.
  static ezUInt32 ReloadResourcesOfType(const ezRTTI* pType, bool bForce);

  /// \brief Goes through all resources and makes sure they are reloaded, if they have changed. If bForce is true, all reloadable resources
  /// are updated, even if there is no indication that they have changed.
  static ezUInt32 ReloadAllResources(bool bForce);

  /// \brief Calls ezResource::ResetResource() on all resources.
  ///
  /// This is mostly for usage in tools to reset resource whose state can be modified at runtime, to reset them to their original state.
  static void ResetAllResources();

  // static void CleanUpResources();

  /// \brief Must be called once per frame for some bookkeeping
  static void PerFrameUpdate();

  /// \brief Goes through all existing resources and broadcasts the 'Exists' event.
  /// Used to announce all currently existing resources to interested event listeners.
  static void BroadcastExistsEvent();

  /// \brief Sets up a new or existing category of resources.
  ///
  /// Each resource can be assigned to one category. All resources with the same category share the same total memory limits.
  // static void ConfigureResourceCategory(const char* szCategoryName, ezUInt64 uiMemoryLimitCPU, ezUInt64 uiMemoryLimitGPU);

  // static const ezResourceCategory& GetResourceCategory(const char* szCategoryName);

  /// \brief Registers a 'named' resource. When a resource is looked up using \a szLookupName, the lookup will be redirected to \a
  /// szRedirectionResource.
  ///
  /// This can be used to register a resource under an easier to use name. For example one can register "MenuBackground" as the name for "{
  /// E50DCC85-D375-4999-9CFE-42F1377FAC85 }". If the lookup name already exists, it will be overwritten.
  static void RegisterNamedResource(const char* szLookupName, const char* szRedirectionResource);

  /// \brief Removes a previously registered name from the redirection table.
  static void UnregisterNamedResource(const char* szLookupName);

  /// \brief Returns the resource manager mutex. Allows to lock the manager on a thread when multiple operations need to be done in
  /// sequence.
  static ezMutex& GetMutex() { return s_ResourceMutex; }

  /// \brief Calls ReloadResource on the given resource, but makes sure that the reload happens with the given custom loader.
  ///
  /// Use this e.g. with a ezResourceLoaderFromMemory to replace an existing resource with new data that was created on-the-fly.
  static void UpdateResourceWithCustomLoader(const ezTypelessResourceHandle& hResource, ezUniquePtr<ezResourceTypeLoader>&& loader);

  /// \brief Makes sure all resources that are currently in the preload queue, are finished loading.
  ///
  /// Returns whether any resources were waited for.
  /// \note This will only wait for the preload queue to be empty at this point in time. It does not mean that all resources
  /// are fully loaded (all levels-of-detail). Once you render a frame, more resources might end up in the preload queue again.
  /// So if one wants to make a full detail screenshot and have all resources loaded with all details, one must render multiple frames
  /// and only make a screenshot once no resources where waited for anymore.
  static bool FinishLoadingOfResources();

  /// \brief Makes sure that no further resource loading will take place.
  static void EngineAboutToShutdown();

  static void SetResourceLowResData(const ezTypelessResourceHandle& hResource, ezStreamReader* pStream);

  /// \name Resource Type Overrides
  ///@{

public:
  /// \brief Registers a resource type to be used instead of any of it's base classes, when loading specific data
  ///
  /// When resource B is derived from A it can be registered to be instantiated when loading data, even if the code specifies to use a
  /// resource of type A.
  /// Whenever LoadResource<A>() is executed, the registered callback \a OverrideDecider is run to figure out whether B should be
  /// instantiated instead. If OverrideDecider returns true, B is used.
  ///
  /// OverrideDecider is given the resource ID after it has been resolved by the ezFileSystem. So it has to be able to make its decision
  /// from the file path, name or extension.
  /// The override is registered for all base classes of \a pDerivedTypeToUse, in case the derivation hierarchy is longer.
  ///
  /// Without calling this at startup, a derived resource type has to be manually requested in code.
  static void RegisterResourceOverrideType(const ezRTTI* pDerivedTypeToUse, ezDelegate<bool(const ezStringBuilder&)> OverrideDecider);

  /// \brief Unregisters \a pDerivedTypeToUse as an override resource
  ///
  /// \sa RegisterResourceOverrideType()
  static void UnregisterResourceOverrideType(const ezRTTI* pDerivedTypeToUse);

private:
  struct DerivedTypeInfo
  {
    const ezRTTI* m_pDerivedType = nullptr;
    ezDelegate<bool(const ezStringBuilder&)> m_Decider;
  };

  /// \brief Checks whether there is a type override for pRtti given szResourceID and returns that
  static const ezRTTI* FindResourceTypeOverride(const ezRTTI* pRtti, const char* szResourceID);

  static ezMap<const ezRTTI*, ezHybridArray<DerivedTypeInfo, 4>> s_DerivedTypeInfos;

  ///@}
  /// \name Resource Fallbacks
  ///@{
public:
  template <typename RESOURCE_TYPE>
  static void SetResourceTypeLoadingFallback(const ezTypedResourceHandle<RESOURCE_TYPE>& hResource)
  {
    RESOURCE_TYPE::SetResourceTypeLoadingFallback(hResource);
  }

  template <typename RESOURCE_TYPE>
  static const ezTypedResourceHandle<RESOURCE_TYPE>& GetResourceTypeLoadingFallback()
  {
    return RESOURCE_TYPE::GetResourceTypeLoadingFallback();
  }

  template <typename RESOURCE_TYPE>
  static void SetResourceTypeMissingFallback(const ezTypedResourceHandle<RESOURCE_TYPE>& hResource)
  {
    RESOURCE_TYPE::SetResourceTypeMissingFallback(hResource);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    if (hResource.IsValid())
    {
      // TODO (resources): ezResourceLock is unknown here (declared below)
      // ezResourceLock<RESOURCE_TYPE> lock(hResource, ezResourceAcquireMode::NoFallback);
      /* if this fails, the 'missing resource' is missing itself*/
    }
#endif
  }

  template <typename RESOURCE_TYPE>
  static const ezTypedResourceHandle<RESOURCE_TYPE>& GetResourceTypeMissingFallback()
  {
    return RESOURCE_TYPE::GetResourceTypeMissingFallback();
  }

  using ResourceCleanupCB = ezDelegate<void()>;

  static void AddResourceCleanupCallback(ResourceCleanupCB cb);

  static void ClearResourceCleanupCallback(ResourceCleanupCB cb);

  /// \brief This will clear ALL resources that were registered as 'missing' or 'loading' fallback resources. This is called early during
  /// system shutdown to clean up resources.
  static void ExecuteAllResourceCleanupCallbacks();

private:


  static ezDynamicArray<ResourceCleanupCB> s_ResourceCleanupCallbacks;

  ///@}



private:
  friend class ezResource;
  friend class ezResourceManagerWorkerDiskRead;
  friend class ezResourceManagerWorkerMainThread;
  friend class ezResourceHandleReadContext;

public:
  // TODO (resources): hide
  static void BroadcastResourceEvent(const ezResourceEvent& e);

private:
  static void PluginEventHandler(const ezPlugin::PluginEvent& e);

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Core, ResourceManager);
  static void OnEngineShutdown();
  static void OnCoreShutdown();
  static void OnCoreStartup();

  static void EnsureResourceLoadingState(ezResource* pResource, const ezResourceState RequestedState);


  static bool HelpResourceLoading();

  static bool ReloadResource(ezResource* pResource, bool bForce);

  static void PreloadResource(ezResource* pResource, ezTime tShouldBeAvailableIn);

  template <typename ResourceType>
  static ResourceType* GetResource(const char* szResourceID, bool bIsReloadable);

  static ezResource* GetResource(const ezRTTI* pRtti, const char* szResourceID, bool bIsReloadable);

  static void InternalPreloadResource(ezResource* pResource, bool bHighestPriority);

  static void RunWorkerTask(ezResource* pResource);

  static void UpdateLoadingDeadlines();

  static ezResourceTypeLoader* GetResourceTypeLoader(const ezRTTI* pRTTI);

  struct LoadedResources
  {
    ezHashTable<ezTempHashedString, ezResource*> m_Resources;
  };

  static ezHashTable<const ezRTTI*, LoadedResources> s_LoadedResources;
  static ezMap<ezString, ezResourceTypeLoader*> m_ResourceTypeLoader;

  static ezResourceLoaderFromFile m_FileResourceLoader;

  static ezResourceTypeLoader* m_pDefaultResourceLoader;

  struct LoadingInfo
  {
    ezTime m_DueDate;
    ezResource* m_pResource;

    EZ_ALWAYS_INLINE bool operator==(const LoadingInfo& rhs) const { return m_pResource == rhs.m_pResource; }

    inline bool operator<(const LoadingInfo& rhs) const
    {
      if (m_DueDate < rhs.m_DueDate)
        return true;
      if (m_DueDate > rhs.m_DueDate)
        return false;

      return m_pResource < rhs.m_pResource;
    }
  };

  // this is the resource preload queue
  static ezDeque<LoadingInfo> m_RequireLoading;

  static const ezUInt32 MaxDiskReadTasks = 2;
  static const ezUInt32 MaxMainThreadTasks = 16;
  static bool m_bTaskRunning;
  static bool m_bStop;
  static ezResourceManagerWorkerDiskRead m_WorkerTasksDiskRead[MaxDiskReadTasks];
  static ezResourceManagerWorkerMainThread m_WorkerTasksMainThread[MaxMainThreadTasks];
  static ezUInt8 m_iCurrentWorkerMainThread;
  static ezUInt8 m_iCurrentWorkerDiskRead;
  static ezTime m_LastDeadLineUpdate;
  static ezTime m_LastFrameUpdate;
  static bool m_bBroadcastExistsEvent;
  // static ezHashTable<ezUInt32, ezResourceCategory> m_ResourceCategories;
  static ezMutex s_ResourceMutex;
  static ezHashTable<ezTempHashedString, ezHashedString> s_NamedResources;
  static ezMap<ezString, const ezRTTI*> s_AssetToResourceType;
  static ezMap<ezResource*, ezUniquePtr<ezResourceTypeLoader>> s_CustomLoaders;
  static ezAtomicInteger32 s_ResourcesLoadedRecently;
  static ezAtomicInteger32 s_ResourcesInLoadingLimbo; // not in the loading queue anymore but not yet finished loading either (typically now
                                                      // a task in the task system)
};

/// \brief Helper class to acquire and release a resource safely.
///
/// The constructor calls ezResourceManager::BeginAcquireResource, the destructor makes sure to call ezResourceManager::EndAcquireResource.
/// The instance of this class can be used like a pointer to the resource.
template <class RESOURCE_TYPE>
class ezResourceLock
{
public:
  ezResourceLock(const ezTypedResourceHandle<RESOURCE_TYPE>& hResource, ezResourceAcquireMode mode = ezResourceAcquireMode::AllowFallback,
                 const ezTypedResourceHandle<RESOURCE_TYPE>& hFallbackResource = ezTypedResourceHandle<RESOURCE_TYPE>(),
                 ezResourcePriority Priority = ezResourcePriority::Unchanged)
  {
    m_pResource = ezResourceManager::BeginAcquireResource(hResource, mode, hFallbackResource, Priority, &m_AcquireResult);
  }

  ~ezResourceLock()
  {
    if (m_pResource)
      ezResourceManager::EndAcquireResource(m_pResource);
  }

  RESOURCE_TYPE* operator->() { return m_pResource; }

  operator bool() { return m_pResource != nullptr; }

  ezResourceAcquireResult GetAcquireResult() const { return m_AcquireResult; }

private:
  ezResourceAcquireResult m_AcquireResult;
  RESOURCE_TYPE* m_pResource;
};

#include <Core/ResourceManager/Implementation/ResourceManager_inl.h>
