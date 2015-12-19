#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <ToolsFoundation/Basics/Status.h>

class ezRenderPipelineAssetManager : public ezAssetDocumentManager
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineAssetManager, ezAssetDocumentManager);

public:
  ezRenderPipelineAssetManager();
  ~ezRenderPipelineAssetManager();

  virtual ezString GetResourceTypeExtension() const override { return "ezRenderPipeline"; }

  virtual void QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const override
  {
    inout_AssetTypeNames.Insert("RenderPipeline");
  }

private:
  void OnDocumentManagerEvent(const ezDocumentManager::Event& e);

  virtual ezStatus InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const;
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument);
  virtual void InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const;

};

