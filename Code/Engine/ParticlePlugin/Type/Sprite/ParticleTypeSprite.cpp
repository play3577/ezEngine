#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Type/Sprite/ParticleTypeSprite.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <Core/World/GameObject.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/World/World.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSpriteAxis, 1)
EZ_ENUM_CONSTANTS(ezSpriteAxis::Random, ezSpriteAxis::EmitterX, ezSpriteAxis::EmitterY, ezSpriteAxis::EmitterZ, ezSpriteAxis::WorldX, ezSpriteAxis::WorldY, ezSpriteAxis::WorldZ)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeSpriteFactory, 1, ezRTTIDefaultAllocator<ezParticleTypeSpriteFactory>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_ENUM_MEMBER_PROPERTY("Rotation Axis", ezSpriteAxis, m_RotationAxis),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleTypeSprite, 1, ezRTTIDefaultAllocator<ezParticleTypeSprite>)
EZ_END_DYNAMIC_REFLECTED_TYPE

const ezRTTI* ezParticleTypeSpriteFactory::GetTypeType() const
{
  return ezGetStaticRTTI<ezParticleTypeSprite>();
}


void ezParticleTypeSpriteFactory::CopyTypeProperties(ezParticleType* pObject) const
{
  ezParticleTypeSprite* pType = static_cast<ezParticleTypeSprite*>(pObject);

  pType->m_RotationAxis = m_RotationAxis;
  pType->m_hTexture.Invalidate();

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = ezResourceManager::LoadResource<ezTextureResource>(m_sTexture);
}

enum class TypeSpriteVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added texture
  Version_3, // added Sprite rotation mode

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleTypeSpriteFactory::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = (int)TypeSpriteVersion::Version_Current;
  stream << uiVersion;

  stream << m_sTexture;
  stream << m_RotationAxis.GetValue();
}

void ezParticleTypeSpriteFactory::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)TypeSpriteVersion::Version_Current, "Invalid version %u", uiVersion);

  if (uiVersion >= 2)
  {
    stream >> m_sTexture;
  }

  if (uiVersion >= 3)
  {
    ezSpriteAxis::StorageType val;
    stream >> val;
    m_RotationAxis.SetValue(val);
  }
}


ezParticleTypeSprite::ezParticleTypeSprite()
{
  m_uiLastExtractedFrame = 0;
}

void ezParticleTypeSprite::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float3, &m_pStreamPosition, false);
  CreateStream("Size", ezProcessingStream::DataType::Float, &m_pStreamSize, false);
  CreateStream("Color", ezProcessingStream::DataType::Float4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", ezProcessingStream::DataType::Float, &m_pStreamRotationSpeed, false);
}

void ezParticleTypeSprite::ExtractTypeRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& instanceTransform, ezUInt64 uiExtractedFrame) const
{
  if (!m_hTexture.IsValid())
    return;

  if (GetOwnerSystem()->GetNumActiveParticles() == 0)
    return;

  const ezTime tCur = GetOwnerSystem()->GetWorld()->GetClock().GetAccumulatedTime();

  const ezVec3 vEmitterPos = GetOwnerSystem()->GetTransform().m_vPosition;
  const ezVec3 vEmitterDir = ezVec3(0, 0, 1);// GetOwnerSystem()->GetTransform().m_Rotation.GetColumn(2).GetNormalized(); // Z axis

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != uiExtractedFrame)
  {
    m_uiLastExtractedFrame = uiExtractedFrame;

    const ezVec3* pPosition = m_pStreamPosition->GetData<ezVec3>();
    const float* pSize = m_pStreamSize->GetData<float>();
    const ezColor* pColor = m_pStreamColor->GetData<ezColor>();
    const float* pRotationSpeed = m_pStreamRotationSpeed->GetData<float>();

    if (m_GpuData == nullptr)
    {
      m_GpuData = EZ_DEFAULT_NEW(ezSpriteParticleDataContainer);
      m_GpuData->m_Content.SetCountUninitialized((ezUInt32)GetOwnerSystem()->GetMaxParticles());
    }

    ezSpriteParticleData* TempData = m_GpuData->m_Content.GetData();

    ezTransform t;

    for (ezUInt32 p = 0; p < (ezUInt32)GetOwnerSystem()->GetNumActiveParticles(); ++p)
    {
      TempData[p].Size = pSize[p];
      TempData[p].Color = pColor[p];
    }

    if (m_RotationAxis == ezSpriteAxis::EmitterZ)
    {
      const ezVec3 vTangentZ = vEmitterDir;
      const ezVec3 vTangentX = vEmitterDir.GetOrthogonalVector();

      for (ezUInt32 p = 0; p < (ezUInt32)GetOwnerSystem()->GetNumActiveParticles(); ++p)
      {
        ezMat3 mRotation;
        mRotation.SetRotationMatrix(vEmitterDir, ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[p])));

        TempData[p].Position = pPosition[p];
        TempData[p].TangentX = mRotation * vTangentX;
        TempData[p].TangentZ = vTangentZ;
      }
    }
    else if (m_RotationAxis == ezSpriteAxis::WorldZ)
    {
      const ezVec3 vTangentZ = vEmitterDir;
      const ezVec3 vTangentX = vEmitterDir.GetOrthogonalVector();

      for (ezUInt32 p = 0; p < (ezUInt32)GetOwnerSystem()->GetNumActiveParticles(); ++p)
      {
        const ezVec3 vDirToParticle = (pPosition[p] - vEmitterPos);
        const ezVec3 vOrthoDir = vEmitterDir.Cross(vDirToParticle).GetNormalized();

        ezMat3 mRotation;
        mRotation.SetRotationMatrix(vOrthoDir, ezAngle::Radian((float)(tCur.GetSeconds() * pRotationSpeed[p])));

        TempData[p].Position = pPosition[p];
        TempData[p].TangentX = vOrthoDir;
        TempData[p].TangentZ = mRotation * vTangentZ;
      }
    }
  }

  /// \todo Is this batch ID correct?
  const ezUInt32 uiBatchId = m_hTexture.GetResourceIDHash();
  auto pRenderData = ezCreateRenderDataForThisFrame<ezParticleSpriteRenderData>(nullptr, uiBatchId);

  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_uiNumParticles = (ezUInt32)GetOwnerSystem()->GetNumActiveParticles();
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_GpuData = m_GpuData;

  /// \todo Generate a proper sorting key?
  const ezUInt32 uiSortingKey = 0;
  pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::SimpleTransparent, uiSortingKey);
}

