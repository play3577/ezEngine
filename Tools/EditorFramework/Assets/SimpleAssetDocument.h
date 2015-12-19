#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>

template<typename PropertyType>
class ezSimpleAssetDocument : public ezAssetDocument
{
public:
  ezSimpleAssetDocument(const char* szDocumentPath) : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezSimpleDocumentObjectManager<PropertyType>))
  {

  }

  ~ezSimpleAssetDocument()
  {
    m_ObjectMirror.Clear();
    m_ObjectMirror.DeInit();
  }

  const PropertyType* GetProperties() const
  {
    return static_cast<const PropertyType*>(m_ObjectMirror.GetNativeObjectPointer(GetObjectManager()->GetRootObject()->GetChildren()[0]));
  }

  PropertyType* GetProperties()
  {
    return static_cast<PropertyType*>(m_ObjectMirror.GetNativeObjectPointer(GetObjectManager()->GetRootObject()->GetChildren()[0]));
  }

  ezDocumentObject* GetPropertyObject()
  {
    return GetObjectManager()->GetRootObject()->GetChildren()[0];
  }

protected:
  virtual void InitializeAfterLoading() override
  {
    ezAssetDocument::InitializeAfterLoading();

    EnsureSettingsObjectExist();

    m_ObjectMirror.InitSender(GetObjectManager());
    m_ObjectMirror.InitReceiver(&m_Context);
    m_ObjectMirror.SendDocument();
  }

  virtual ezStatus InternalLoadDocument() override
  {
    GetObjectManager()->DestroyAllObjects();

    ezStatus ret = ezAssetDocument::InternalLoadDocument();

    return ret;
  }

private:
  void EnsureSettingsObjectExist()
  {
    auto pRoot = GetObjectManager()->GetRootObject();
    if (pRoot->GetChildren().IsEmpty())
    {
      ezDocumentObject* pObject = GetObjectManager()->CreateObject(ezGetStaticRTTI<PropertyType>());
      GetObjectManager()->AddObject(pObject, pRoot, "Children", 0);
    }
  }

  virtual ezDocumentInfo* CreateDocumentInfo() override 
  { 
    return EZ_DEFAULT_NEW(ezAssetDocumentInfo); 
  }

  ezDocumentObjectMirror m_ObjectMirror;
  ezRttiConverterContext m_Context;
};




template<typename ObjectProperties>
class ezSimpleDocumentObjectManager : public ezDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const override
  {
    Types.PushBack(ezGetStaticRTTI<ObjectProperties>());
  }

};