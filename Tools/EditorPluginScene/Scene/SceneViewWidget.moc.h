#pragma once

#include <Foundation/Basics.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginScene/InputContexts/SelectionContext.h>
#include <EditorPluginScene/InputContexts/CameraMoveContext.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

class QVBoxLayout;
class ezQtSceneDocumentWindow;
class ezOrthoGizmoContext;

class ezQtSceneViewWidget : public ezQtEngineViewWidget
{
  Q_OBJECT
public:
  ezQtSceneViewWidget(QWidget* pParent, ezQtSceneDocumentWindow* pDocument, ezCameraMoveContextSettings* pCameraMoveSettings, ezSceneViewConfig* pViewConfig);
  ~ezQtSceneViewWidget();

  ezOrthoGizmoContext* m_pOrthoGizmoContext;
  ezSelectionContext* m_pSelectionContext;
  ezCameraMoveContext* m_pCameraMoveContext;

  virtual void SyncToEngine() override;

  virtual bool IsPickingAgainstSelectionAllowed() const override;

protected:
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* e) override;
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;

  bool m_bAllowPickSelectedWhileDragging;
  ezTime m_LastDragMoveEvent;
};
