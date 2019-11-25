#pragma once

#include <PhysXPlugin/Components/PxComponent.h>

struct ezMsgExtractGeometry;

class EZ_PHYSXPLUGIN_DLL ezPxActorComponent : public ezPxComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxActorComponent, ezPxComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPxActorComponent

public:
  ezPxActorComponent();
  ~ezPxActorComponent();

protected:
  void AddShapesFromObject(ezGameObject* pObject, physx::PxRigidActor* pActor, const ezSimdTransform& ParentTransform);
  void AddShapesToNavMesh(const ezGameObject* pObject, ezMsgExtractGeometry& msg) const;
};
