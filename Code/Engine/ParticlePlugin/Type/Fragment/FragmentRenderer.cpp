#include <PCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Type/Fragment/FragmentRenderer.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/../../../Data/Base/Shaders/Particles/ParticleSystemConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFragmentRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleFragmentRenderer, 1, ezRTTIDefaultAllocator<ezParticleFragmentRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleFragmentRenderer::~ezParticleFragmentRenderer()
{
  if (!m_hBaseDataBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hBaseDataBuffer);
    m_hBaseDataBuffer.Invalidate();
  }

  if (!m_hQuadDataBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hQuadDataBuffer);
    m_hQuadDataBuffer.Invalidate();
  }
}

void ezParticleFragmentRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezParticleFragmentRenderData>());
}

void ezParticleFragmentRenderer::CreateDataBuffer()
{
  if (m_hBaseDataBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezBaseParticleShaderData);
    desc.m_uiTotalSize = s_uiParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hBaseDataBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  if (m_hQuadDataBuffer.IsInvalidated())
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezTangentQuadParticleShaderData);
    desc.m_uiTotalSize = s_uiParticlesPerBatch * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hQuadDataBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }
}

void ezParticleFragmentRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass,
                                             const ezRenderDataBatch& batch)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  /// \todo This pattern looks like it is inefficient. Should it use the GPU pool instead somehow?
  // prepare the constant buffer
  ezConstantBufferStorage<ezParticleSystemConstants>* pConstantBuffer;
  ezConstantBufferStorageHandle hConstantBuffer = ezRenderContext::CreateConstantBufferStorage(pConstantBuffer);
  EZ_SCOPE_EXIT(ezRenderContext::DeleteConstantBufferStorage(hConstantBuffer));
  renderViewContext.m_pRenderContext->BindConstantBuffer("ezParticleSystemConstants", hConstantBuffer);

  // Bind the particle shader
  {
    if (!m_hShader.IsValid())
    {
      m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Particles/QuadParticle.ezShader");
    }

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_QUAD_MODE", "PARTICLE_QUAD_MODE_TANGENTS");
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_RENDER_MODE", "PARTICLE_RENDER_MODE_OPAQUE");
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PARTICLE_OUTPUT_MODE", "PARTICLE_OUTPUT_MODE_DEFAULT");
  }

  // make sure our structured buffer is allocated and bound
  {
    CreateDataBuffer();
    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles,
                                                       s_uiParticlesPerBatch * 2);

    renderViewContext.m_pRenderContext->BindBuffer("particleBaseData", pDevice->GetDefaultResourceView(m_hBaseDataBuffer));
    renderViewContext.m_pRenderContext->BindBuffer("particleTangentQuadData", pDevice->GetDefaultResourceView(m_hQuadDataBuffer));
  }

  // now render all particle effects of type Fragment
  for (auto it = batch.GetIterator<ezParticleFragmentRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const ezParticleFragmentRenderData* pRenderData = it;
    ezUInt32 uiNumParticles = pRenderData->m_BaseParticleData.GetCount();

    renderViewContext.m_pRenderContext->BindTexture2D("ParticleTexture", pRenderData->m_hTexture);

    // fill the constant buffer
    {
      ezParticleSystemConstants& cb = pConstantBuffer->GetDataForWriting();
      cb.NumSpritesX = pRenderData->m_uiNumSpritesX;
      cb.NumSpritesY = pRenderData->m_uiNumSpritesY;

      if (pRenderData->m_bApplyObjectTransform)
        cb.ObjectToWorldMatrix = pRenderData->m_GlobalTransform.GetAsMat4();
      else
        cb.ObjectToWorldMatrix.SetIdentity();
    }

    const ezBaseParticleShaderData* pParticleBaseData = pRenderData->m_BaseParticleData.GetPtr();
    const ezTangentQuadParticleShaderData* pParticleQuadData = pRenderData->m_QuadParticleData.GetPtr();

    while (uiNumParticles > 0)
    {
      // upload this batch of particle data
      const ezUInt32 uiNumParticlesInBatch = ezMath::Min<ezUInt32>(uiNumParticles, s_uiParticlesPerBatch);

      pGALContext->UpdateBuffer(m_hBaseDataBuffer, 0, ezMakeArrayPtr(pParticleBaseData, uiNumParticlesInBatch).ToByteArray());
      pGALContext->UpdateBuffer(m_hQuadDataBuffer, 0, ezMakeArrayPtr(pParticleQuadData, uiNumParticlesInBatch).ToByteArray());

      // do one drawcall
      renderViewContext.m_pRenderContext->DrawMeshBuffer(uiNumParticlesInBatch * 2);

      uiNumParticles -= uiNumParticlesInBatch;
      pParticleBaseData += uiNumParticlesInBatch;
      pParticleQuadData += uiNumParticlesInBatch;
    }
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Fragment_FragmentRenderer);
