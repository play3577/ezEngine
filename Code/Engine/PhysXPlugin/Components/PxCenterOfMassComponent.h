#pragma once

#include <PhysXPlugin/Components/PxActorComponent.h>
#include <PhysXPlugin/Resources/PxMeshResource.h>

typedef ezComponentManager<class ezPxCenterOfMassComponent> ezPxCenterOfMassComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxCenterOfMassComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxCenterOfMassComponent, ezPhysXComponent, ezPxCenterOfMassComponentManager);

public:
  ezPxCenterOfMassComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:


protected:

  // ************************************* FUNCTIONS *****************************

public:

private:

};
