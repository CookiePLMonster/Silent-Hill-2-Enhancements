/**
* Copyright (C) 2020 Elisha Riedlinger
*
* This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
* authors be held liable for any damages arising from the use of this software.
* Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
*      original  software. If you use this  software  in a product, an  acknowledgment in the product
*      documentation would be appreciated but is not required.
*   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
*      being the original software.
*   3. This notice may not be removed or altered from any source distribution.
*/

#include "d3d8wrapper.h"
#include "Common\Utils.h"

bool IsInBloomEffect = false;
bool ClassReleaseFlag = false;

HRESULT m_IDirect3DDevice8::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
	Logging::LogDebug() << __FUNCTION__;

	if ((riid == IID_IDirect3DDevice8 || riid == IID_IUnknown) && ppvObj)
	{
		AddRef();

		*ppvObj = this;

		return S_OK;
	}

	HRESULT hr = ProxyInterface->QueryInterface(riid, ppvObj);

	if (SUCCEEDED(hr))
	{
		genericQueryInterface(riid, ppvObj, this);
	}

	return hr;
}

ULONG m_IDirect3DDevice8::AddRef()
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->AddRef();
}

ULONG m_IDirect3DDevice8::Release()
{
	Logging::LogDebug() << __FUNCTION__;

	ULONG count = ProxyInterface->Release();

	if (count == 0)
	{
		delete this;
	}

	return count;
}

HRESULT m_IDirect3DDevice8::Reset(D3DPRESENT_PARAMETERS *pPresentationParameters)
{
	Logging::LogDebug() << __FUNCTION__;

	if (pInSurface)
	{
		ReleaseInterface(&pInSurface, 2);
	}

	if (pInTexture)
	{
		ReleaseInterface(&pInTexture);
	}

	if (pShrunkSurface)
	{
		ReleaseInterface(&pShrunkSurface, 2);
	}

	if (pShrunkTexture)
	{
		ReleaseInterface(&pShrunkTexture);
	}

	if (pOutSurface)
	{
		ReleaseInterface(&pOutSurface, 2);
	}

	if (pOutTexture)
	{
		ReleaseInterface(&pOutTexture);
	}

	if (silhouetteSurface)
	{
		ReleaseInterface(&silhouetteSurface, 2);
	}

	if (silhouetteTexture)
	{
		ReleaseInterface(&silhouetteTexture);
	}

	// Update presentation parameters
	UpdatePresentParameter(pPresentationParameters, nullptr, true);

	return ProxyInterface->Reset(pPresentationParameters);
}

HRESULT m_IDirect3DDevice8::EndScene()
{
	Logging::LogDebug() << __FUNCTION__;

	// Skip frames in specific cutscenes to prevent flickering
	if (RemoveEnvironmentFlicker && SH2_CutsceneID && SH2_CutsceneCameraPos && SH2_JamesPos)
	{
		if ((LastCutsceneID == 0x01 && SkipSceneCounter < 4 && (SkipSceneCounter || *SH2_JamesPos != LastJamesPos)) ||
			(LastCutsceneID == 0x03 && SkipSceneCounter < 1 && (SkipSceneCounter || *SH2_JamesPos == 330.845f)) ||
			((LastCutsceneID == 0x15 || LastCutsceneID == 0x16) && SkipSceneCounter < 1 && (SkipSceneCounter || *SH2_CutsceneID != LastCutsceneID || (ClassReleaseFlag && !(*SH2_CutsceneCameraPos == *(float*)"\xAE\x01\x31\x46" && LastCameraPos == 0))) && !(*SH2_CutsceneID == 0x16 && LastCutsceneID == 0x15)) ||
			(LastCutsceneID == 0x16 && SkipSceneCounter < 4 && (SkipSceneCounter || (*SH2_CutsceneCameraPos != LastCameraPos && *SH2_CutsceneCameraPos == *(float*)"\x40\xA1\xA8\x45")) && *SH2_CutsceneID == 0x16) ||
			(LastCutsceneID == 0x4C && SkipSceneCounter < 1 && (SkipSceneCounter || *SH2_CutsceneID != LastCutsceneID)) ||
			(LastCutsceneID == 0x4D && SkipSceneCounter < 2 && (SkipSceneCounter || *SH2_CutsceneID != LastCutsceneID || ClassReleaseFlag)) ||
			(LastCutsceneID == 0x4D && SkipSceneCounter < 3 && (SkipSceneCounter || *SH2_CutsceneCameraPos != LastCameraPos) && *SH2_CutsceneCameraPos == *(float*)"\x59\xCC\x06\xC6" && *SH2_CutsceneID == 0x4D))
		{
			LOG_LIMIT(1, "Skipping frame during cutscene!");
			Logging::LogDebug() << __FUNCTION__ " frame - Counter " << SkipSceneCounter << " Release: " << ClassReleaseFlag << " CutsceneID: " << *SH2_CutsceneID << " LastCutsceneID: " << LastCutsceneID <<
				" CutsceneCameraPos: " << *SH2_CutsceneCameraPos << " LastCameraPos: " << LastCameraPos << " JamesPos: " << *SH2_JamesPos << " LastJamesPos: " << LastJamesPos;

			SkipSceneFlag = true;
			SkipSceneCounter++;

			return D3D_OK;
		}

		SkipSceneFlag = false;
		SkipSceneCounter = 0;
		LastCutsceneID = *SH2_CutsceneID;
		LastCameraPos = *SH2_CutsceneCameraPos;
		LastJamesPos = *SH2_JamesPos;
	}

	EndSceneCounter++;

	return ProxyInterface->EndScene();
}

void m_IDirect3DDevice8::SetCursorPosition(THIS_ UINT XScreenSpace, UINT YScreenSpace, DWORD Flags)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetCursorPosition(XScreenSpace, YScreenSpace, Flags);
}

HRESULT m_IDirect3DDevice8::SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8 *pCursorBitmap)
{
	Logging::LogDebug() << __FUNCTION__;

	if (pCursorBitmap)
	{
		pCursorBitmap = static_cast<m_IDirect3DSurface8 *>(pCursorBitmap)->GetProxyInterface();
	}

	return ProxyInterface->SetCursorProperties(XHotSpot, YHotSpot, pCursorBitmap);
}

BOOL m_IDirect3DDevice8::ShowCursor(BOOL bShow)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->ShowCursor(bShow);
}

HRESULT m_IDirect3DDevice8::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DSwapChain8 **ppSwapChain)
{
	Logging::LogDebug() << __FUNCTION__;

	// Update presentation parameters
	UpdatePresentParameter(pPresentationParameters, nullptr, false);

	HRESULT hr = ProxyInterface->CreateAdditionalSwapChain(pPresentationParameters, ppSwapChain);

	if (SUCCEEDED(hr))
	{
		*ppSwapChain = ProxyAddressLookupTable->FindAddress<m_IDirect3DSwapChain8>(*ppSwapChain);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::CreateCubeTexture(THIS_ UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture);

	if (SUCCEEDED(hr) && ppCubeTexture)
	{
		*ppCubeTexture = ProxyAddressLookupTable->FindAddress<m_IDirect3DCubeTexture8>(*ppCubeTexture);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::CreateDepthStencilSurface(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->CreateDepthStencilSurface(Width, Height, Format, MultiSample, ppSurface);

	if (SUCCEEDED(hr) && ppSurface)
	{
		*ppSurface = ProxyAddressLookupTable->FindAddress<m_IDirect3DSurface8>(*ppSurface);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::CreateIndexBuffer(THIS_ UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer);

	if (SUCCEEDED(hr) && ppIndexBuffer)
	{
		*ppIndexBuffer = ProxyAddressLookupTable->FindAddress<m_IDirect3DIndexBuffer8>(*ppIndexBuffer);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::CreateRenderTarget(THIS_ UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->CreateRenderTarget(Width, Height, Format, MultiSample, Lockable, ppSurface);

	if (SUCCEEDED(hr) && ppSurface)
	{
		*ppSurface = ProxyAddressLookupTable->FindAddress<m_IDirect3DSurface8>(*ppSurface);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::CreateTexture(THIS_ UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8** ppTexture)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture);

	if (SUCCEEDED(hr) && ppTexture)
	{
		*ppTexture = ProxyAddressLookupTable->FindAddress<m_IDirect3DTexture8>(*ppTexture);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::CreateVertexBuffer(THIS_ UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer);

	if (SUCCEEDED(hr) && ppVertexBuffer)
	{
		*ppVertexBuffer = ProxyAddressLookupTable->FindAddress<m_IDirect3DVertexBuffer8>(*ppVertexBuffer);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::CreateVolumeTexture(THIS_ UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture);

	if (SUCCEEDED(hr) && ppVolumeTexture)
	{
		*ppVolumeTexture = ProxyAddressLookupTable->FindAddress<m_IDirect3DVolumeTexture8>(*ppVolumeTexture);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::BeginStateBlock()
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->BeginStateBlock();
}

HRESULT m_IDirect3DDevice8::CreateStateBlock(THIS_ D3DSTATEBLOCKTYPE Type, DWORD* pToken)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->CreateStateBlock(Type, pToken);
}

HRESULT m_IDirect3DDevice8::ApplyStateBlock(THIS_ DWORD Token)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->ApplyStateBlock(Token);
}

HRESULT m_IDirect3DDevice8::CaptureStateBlock(THIS_ DWORD Token)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->CaptureStateBlock(Token);
}

HRESULT m_IDirect3DDevice8::DeleteStateBlock(THIS_ DWORD Token)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->DeleteStateBlock(Token);
}

HRESULT m_IDirect3DDevice8::EndStateBlock(THIS_ DWORD* pToken)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->EndStateBlock(pToken);
}

HRESULT m_IDirect3DDevice8::GetClipStatus(D3DCLIPSTATUS8 *pClipStatus)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetClipStatus(pClipStatus);
}

HRESULT m_IDirect3DDevice8::GetDisplayMode(THIS_ D3DDISPLAYMODE* pMode)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetDisplayMode(pMode);
}

HRESULT m_IDirect3DDevice8::GetRenderState(D3DRENDERSTATETYPE State, DWORD *pValue)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetRenderState(State, pValue);
}

HRESULT m_IDirect3DDevice8::GetRenderTarget(THIS_ IDirect3DSurface8** ppRenderTarget)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->GetRenderTarget(ppRenderTarget);

	if (SUCCEEDED(hr) && ppRenderTarget)
	{
		*ppRenderTarget = ProxyAddressLookupTable->FindAddress<m_IDirect3DSurface8>(*ppRenderTarget);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX *pMatrix)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetTransform(State, pMatrix);
}

HRESULT m_IDirect3DDevice8::SetClipStatus(CONST D3DCLIPSTATUS8 *pClipStatus)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetClipStatus(pClipStatus);
}

HRESULT m_IDirect3DDevice8::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
	Logging::LogDebug() << __FUNCTION__;

	// Fix for 2D Fog and glow around the flashlight lens for Nvidia cards
	if (Fog2DFix && State == D3DRS_ZBIAS)
	{
		Value = (Value * 15) / 16;
	}

	// Restores self shadows
	if (EnableXboxShadows && State == D3DRS_STENCILPASS && Value == D3DSTENCILOP_REPLACE)
	{
		if (SH2_ChapterID && *SH2_ChapterID == 0x01) // Born From a Wish
		{
			if (SH2_SpecializedLight1 && *SH2_SpecializedLight1 != 0x01) // In a specialized lighting zone
			{
				if (SH2_RoomID && *SH2_RoomID != 0x20 && *SH2_RoomID != 0x25 && *SH2_RoomID != 0x26) // Exclude Blue Creek hallways/staircase
				{
					Value = D3DSTENCILOP_ZERO;
				}
			}
		}
		else // Main campaign
		{
			if (SH2_SpecializedLight1 && *SH2_SpecializedLight1 != 0x01 && SH2_SpecializedLight2 && *SH2_SpecializedLight2 != 0x01) // In a specialized lighting zone
			{
				if (SH2_RoomID && *SH2_RoomID != 0x9E) // Exclude Hotel Room 202-204
				{
					Value = D3DSTENCILOP_ZERO;
				}
			}
		}
	}

	// For blur frame flicker fix
	if (RemoveEffectsFlicker && State == D3DRS_TEXTUREFACTOR)
	{
		if (!OverrideTextureLoop && EndSceneCounter != 0)
		{
			OverrideTextureLoop = true;
			PresentFlag = true;
		}
		// Known values to exclude: 0x11000000, 0xFF000000, 0xC31E1E1E, B7222222, B9222222, BA202020, BA1E1E1E, BA1C1C1C, BA1A1A1A, BA171717, BA121212
		if (OverrideTextureLoop && PresentFlag && !((Value & 0xFF) == ((Value >> 8) & 0xFF) && ((Value >> 8) & 0xFF) == ((Value >> 16) & 0xFF)))
		{
			Value = 0xFFFFFFFF;
			IsInBloomEffect = true;
		}
	}

	return ProxyInterface->SetRenderState(State, Value);
}

HRESULT m_IDirect3DDevice8::SetRenderTarget(THIS_ IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil)
{
	Logging::LogDebug() << __FUNCTION__;

	if (pRenderTarget)
	{
		pRenderTarget = static_cast<m_IDirect3DSurface8 *>(pRenderTarget)->GetProxyInterface();
	}

	if (pNewZStencil)
	{
		pNewZStencil = static_cast<m_IDirect3DSurface8 *>(pNewZStencil)->GetProxyInterface();
	}

	return ProxyInterface->SetRenderTarget(pRenderTarget, pNewZStencil);
}

HRESULT m_IDirect3DDevice8::SetTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX *pMatrix)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetTransform(State, pMatrix);
}

void m_IDirect3DDevice8::GetGammaRamp(THIS_ D3DGAMMARAMP* pRamp)
{
	Logging::LogDebug() << __FUNCTION__;

	ProxyInterface->GetGammaRamp(pRamp);
}

void m_IDirect3DDevice8::SetGammaRamp(THIS_ DWORD Flags, CONST D3DGAMMARAMP* pRamp)
{
	Logging::LogDebug() << __FUNCTION__;

	if (!EnableWndMode || FullscreenWndMode)
	{
		ProxyInterface->SetGammaRamp(Flags, pRamp);
	}
}

HRESULT m_IDirect3DDevice8::DeletePatch(UINT Handle)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->DeletePatch(Handle);
}

HRESULT m_IDirect3DDevice8::DrawRectPatch(UINT Handle, CONST float *pNumSegs, CONST D3DRECTPATCH_INFO *pRectPatchInfo)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
}

HRESULT m_IDirect3DDevice8::DrawTriPatch(UINT Handle, CONST float *pNumSegs, CONST D3DTRIPATCH_INFO *pTriPatchInfo)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);
}

HRESULT m_IDirect3DDevice8::GetIndices(THIS_ IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->GetIndices(ppIndexData, pBaseVertexIndex);

	if (SUCCEEDED(hr) && ppIndexData)
	{
		*ppIndexData = ProxyAddressLookupTable->FindAddress<m_IDirect3DIndexBuffer8>(*ppIndexData);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::SetIndices(THIS_ IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex)
{
	Logging::LogDebug() << __FUNCTION__;

	if (pIndexData)
	{
		pIndexData = static_cast<m_IDirect3DIndexBuffer8 *>(pIndexData)->GetProxyInterface();
	}

	return ProxyInterface->SetIndices(pIndexData, BaseVertexIndex);
}

UINT m_IDirect3DDevice8::GetAvailableTextureMem()
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetAvailableTextureMem();
}

HRESULT m_IDirect3DDevice8::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetCreationParameters(pParameters);
}

HRESULT m_IDirect3DDevice8::GetDeviceCaps(D3DCAPS8 *pCaps)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetDeviceCaps(pCaps);
}

HRESULT m_IDirect3DDevice8::GetDirect3D(IDirect3D8 **ppD3D9)
{
	Logging::LogDebug() << __FUNCTION__;

	if (!ppD3D9)
	{
		return D3DERR_INVALIDCALL;
	}

	m_pD3D->AddRef();

	*ppD3D9 = m_pD3D;

	return D3D_OK;
}

HRESULT m_IDirect3DDevice8::GetRasterStatus(THIS_ D3DRASTER_STATUS* pRasterStatus)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetRasterStatus(pRasterStatus);
}

HRESULT m_IDirect3DDevice8::GetLight(DWORD Index, D3DLIGHT8 *pLight)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetLight(Index, pLight);
}

HRESULT m_IDirect3DDevice8::GetLightEnable(DWORD Index, BOOL *pEnable)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetLightEnable(Index, pEnable);
}

HRESULT m_IDirect3DDevice8::GetMaterial(D3DMATERIAL8 *pMaterial)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetMaterial(pMaterial);
}

HRESULT m_IDirect3DDevice8::LightEnable(DWORD LightIndex, BOOL bEnable)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->LightEnable(LightIndex, bEnable);
}

HRESULT m_IDirect3DDevice8::SetLight(DWORD Index, CONST D3DLIGHT8 *pLight)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetLight(Index, pLight);
}

HRESULT m_IDirect3DDevice8::SetMaterial(CONST D3DMATERIAL8 *pMaterial)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetMaterial(pMaterial);
}

HRESULT m_IDirect3DDevice8::MultiplyTransform(D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX *pMatrix)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->MultiplyTransform(State, pMatrix);
}

HRESULT m_IDirect3DDevice8::ProcessVertices(THIS_ UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags)
{
	Logging::LogDebug() << __FUNCTION__;

	if (pDestBuffer)
	{
		pDestBuffer = static_cast<m_IDirect3DVertexBuffer8 *>(pDestBuffer)->GetProxyInterface();
	}

	return ProxyInterface->ProcessVertices(SrcStartIndex, DestIndex, VertexCount, pDestBuffer, Flags);
}

HRESULT m_IDirect3DDevice8::TestCooperativeLevel()
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->TestCooperativeLevel();
}

HRESULT m_IDirect3DDevice8::GetCurrentTexturePalette(UINT *pPaletteNumber)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetCurrentTexturePalette(pPaletteNumber);
}

HRESULT m_IDirect3DDevice8::GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY *pEntries)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetPaletteEntries(PaletteNumber, pEntries);
}

HRESULT m_IDirect3DDevice8::SetCurrentTexturePalette(UINT PaletteNumber)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetCurrentTexturePalette(PaletteNumber);
}

HRESULT m_IDirect3DDevice8::SetPaletteEntries(UINT PaletteNumber, CONST PALETTEENTRY *pEntries)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetPaletteEntries(PaletteNumber, pEntries);
}

HRESULT m_IDirect3DDevice8::CreatePixelShader(THIS_ CONST DWORD* pFunction, DWORD* pHandle)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->CreatePixelShader(pFunction, pHandle);
}

HRESULT m_IDirect3DDevice8::GetPixelShader(THIS_ DWORD* pHandle)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetPixelShader(pHandle);
}

HRESULT m_IDirect3DDevice8::SetPixelShader(THIS_ DWORD Handle)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetPixelShader(Handle);
}

HRESULT m_IDirect3DDevice8::DeletePixelShader(THIS_ DWORD Handle)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->DeletePixelShader(Handle);
}

HRESULT m_IDirect3DDevice8::GetPixelShaderFunction(THIS_ DWORD Handle, void* pData, DWORD* pSizeOfData)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetPixelShaderFunction(Handle, pData, pSizeOfData);
}

HRESULT m_IDirect3DDevice8::Present(CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion)
{
	Logging::LogDebug() << __FUNCTION__;

	// Skip frames in specific cutscenes to prevent flickering
	if (SkipSceneFlag)
	{
		return D3D_OK;
	}

	// Shadow fading counter
	if (DrawingShadowsFlag)
	{
		ShadowFadingCounter++;
	}

	DrawingShadowsFlag = false;

	// For blur frame flicker fix
	if (EndSceneCounter == 1)
	{
		OverrideTextureLoop = false;
		IsInBloomEffect = false;
	}

	EndSceneCounter = 0;
	PresentFlag = false;

	return ProxyInterface->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

HRESULT m_IDirect3DDevice8::DrawIndexedPrimitive(THIS_ D3DPRIMITIVETYPE Type, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	Logging::LogDebug() << __FUNCTION__;

	// Drawing opaque map geometry and dynamic objects
	if (EnableXboxShadows && !shadowVolumeFlag)
	{
		// Change default states to be more like Xbox version
		ProxyInterface->SetRenderState(D3DRS_STENCILMASK, 0xFFFFFFFF);
		ProxyInterface->SetRenderState(D3DRS_STENCILWRITEMASK, 0xFF);

		// Drawing just opaque map geometry
		DWORD stencilRef = 0;
		if (SUCCEEDED(ProxyInterface->GetRenderState(D3DRS_STENCILREF, &stencilRef)) && stencilRef == 0)
		{
			// Change default states to be more like Xbox version
			ProxyInterface->SetRenderState(D3DRS_STENCILENABLE, TRUE);
			ProxyInterface->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
			ProxyInterface->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_ZERO);
		}
	}

	// Exclude Woodside Room 208 TV static geometry from receiving shadows
	if (EnableSoftShadows && SH2_RoomID && *SH2_RoomID == 0x18 && Type == D3DPT_TRIANGLESTRIP && MinVertexIndex == 0 && NumVertices == 4 && startIndex == 0 && primCount == 2)
	{
		LPDIRECT3DTEXTURE8 texture = nullptr;
		D3DSURFACE_DESC desc = { D3DFMT_UNKNOWN, D3DRTYPE_TEXTURE, 0, D3DPOOL_DEFAULT, 0, D3DMULTISAMPLE_NONE, 0, 0 };

		HRESULT hr = ProxyInterface->GetTexture(0, (LPDIRECT3DBASETEXTURE8 *)&texture);
		if (SUCCEEDED(hr) && texture)
		{
			hr = texture->GetLevelDesc(0, &desc);

			texture->Release();
		}
		if (SUCCEEDED(hr) && desc.Width == 684 && desc.Height == 512)
		{
			DWORD stencilPass = 0;
			ProxyInterface->GetRenderState(D3DRS_STENCILPASS, &stencilPass);

			ProxyInterface->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);

			hr = ProxyInterface->DrawIndexedPrimitive(Type, MinVertexIndex, NumVertices, startIndex, primCount);

			ProxyInterface->SetRenderState(D3DRS_STENCILPASS, stencilPass);

			return hr;
		}
	}
	// Exclude windows in Heaven's Night, Hotel 2F Room Hallway and Hotel Storeroom from receiving shadows
	else if (EnableSoftShadows &&
		((SH2_RoomID && *SH2_RoomID == 0x0C && Type == D3DPT_TRIANGLESTRIP && MinVertexIndex == 0 && NumVertices == 18 && startIndex == 0 && primCount == 21) ||
		(SH2_RoomID && *SH2_RoomID == 0x9F && Type == D3DPT_TRIANGLESTRIP && MinVertexIndex == 0 && NumVertices == 10 && startIndex == 0 && primCount == 10) ||
		(SH2_RoomID && *SH2_RoomID == 0x94 && Type == D3DPT_TRIANGLESTRIP && MinVertexIndex == 0 && NumVertices == 8 && startIndex == 0 && primCount == 8)))
	{
		DWORD stencilPass, stencilRef = 0;

		// Backup renderstates
		ProxyInterface->GetRenderState(D3DRS_STENCILPASS, &stencilPass);
		ProxyInterface->GetRenderState(D3DRS_STENCILREF, &stencilRef);

		// Set states so we don't receive shadows
		ProxyInterface->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
		ProxyInterface->SetRenderState(D3DRS_STENCILREF, 1);

		HRESULT hr = ProxyInterface->DrawIndexedPrimitive(Type, MinVertexIndex, NumVertices, startIndex, primCount);

		// Restore renderstates
		ProxyInterface->SetRenderState(D3DRS_STENCILPASS, stencilPass);
		ProxyInterface->SetRenderState(D3DRS_STENCILREF, stencilRef);

		return hr;
	}
	// Exclude refrigerator interior in hospital from receiving shadows
	else if (EnableSoftShadows && SH2_RoomID && *SH2_RoomID == 0x53 && Type == D3DPT_TRIANGLESTRIP && MinVertexIndex == 0 && NumVertices == 1037 && startIndex == 0 && primCount == 1580)
	{
		DWORD stencilPass = 0;

		// Backup renderstates
		ProxyInterface->GetRenderState(D3DRS_STENCILPASS, &stencilPass);

		// Set states so we don't receive shadows
		ProxyInterface->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);

		HRESULT hr = ProxyInterface->DrawIndexedPrimitive(Type, MinVertexIndex, NumVertices, startIndex, primCount);

		// Restore renderstates
		ProxyInterface->SetRenderState(D3DRS_STENCILPASS, stencilPass);

		return hr;
	}

	return ProxyInterface->DrawIndexedPrimitive(Type, MinVertexIndex, NumVertices, startIndex, primCount);
}

HRESULT m_IDirect3DDevice8::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinIndex, UINT NumVertices, UINT PrimitiveCount, CONST void *pIndexData, D3DFORMAT IndexDataFormat, CONST void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->DrawIndexedPrimitiveUP(PrimitiveType, MinIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT m_IDirect3DDevice8::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
	Logging::LogDebug() << __FUNCTION__;

	// Drawing transparent dynamic objects (Character hair etc.)
	if (EnableXboxShadows && !shadowVolumeFlag)
	{
		// Change default states to be more like Xbox version
		ProxyInterface->SetRenderState(D3DRS_STENCILMASK, 0xFFFFFFFF);
		ProxyInterface->SetRenderState(D3DRS_STENCILWRITEMASK, 0xFF);
	}

	// Disable shadow on the Labyrinth Valve
	if (EnableSoftShadows && SH2_CutsceneID && *SH2_CutsceneID == 0x46 && PrimitiveType == D3DPT_TRIANGLELIST && PrimitiveCount > 496 && PrimitiveCount < 536)
	{
		return D3D_OK;
	}
	// Top Down Shadow
	else if (EnableSoftShadows && ((SH2_RoomID && (*SH2_RoomID == 0x02 || *SH2_RoomID == 0x24 || *SH2_RoomID == 0x8F || *SH2_RoomID == 0x90)) ||
		(SH2_CutsceneID && *SH2_CutsceneID == 0x5A)))
	{
		DWORD stencilPass = 0;
		ProxyInterface->GetRenderState(D3DRS_STENCILPASS, &stencilPass);

		if (stencilPass == D3DSTENCILOP_INCR)
		{
			DWORD stencilFunc = 0;
			ProxyInterface->GetRenderState(D3DRS_STENCILFUNC, &stencilFunc);

			ProxyInterface->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
			ProxyInterface->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_LESS);

			HRESULT hr = ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);

			ProxyInterface->SetRenderState(D3DRS_STENCILPASS, stencilPass);
			ProxyInterface->SetRenderState(D3DRS_STENCILFUNC, stencilFunc);

			return hr;
		}
	}

	DWORD stencilPass = 0;
	ProxyInterface->GetRenderState(D3DRS_STENCILPASS, &stencilPass);

	if (EnableXboxShadows && stencilPass == D3DSTENCILOP_INCR && !shadowVolumeFlag)
	{
		// Drawing shadow volumes, toggle flag on
		shadowVolumeFlag = true;

		IDirect3DSurface8 *backbufferColor, *backbufferDepthStencil = nullptr;

		// Backup backbuffer and depth/stencil buffer
		if (SUCCEEDED(ProxyInterface->GetRenderTarget(&backbufferColor)) && backbufferColor)
		{
			backbufferColor->Release();
		}
		if (SUCCEEDED(ProxyInterface->GetDepthStencilSurface(&backbufferDepthStencil)) && backbufferDepthStencil)
		{
			backbufferDepthStencil->Release();
		}

		// Create texture for James' silhouette if it doesn't exist
		if (!silhouetteTexture)
		{
			if (SUCCEEDED(ProxyInterface->CreateTexture(BufferWidth, BufferHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &silhouetteTexture)) && silhouetteTexture)
			{
				silhouetteTexture->GetSurfaceLevel(0, &silhouetteSurface);
			}
		}

		// DrawPrimitiveUP trashes vertex buffer stream 0, back it up
		IDirect3DVertexBuffer8 *pStream0;
		UINT stream0Stride = 0;
		if (SUCCEEDED(ProxyInterface->GetStreamSource(0, &pStream0, &stream0Stride)) && pStream0)
		{
			pStream0->Release();
		}

		// Error checking
		if (!silhouetteTexture || !silhouetteSurface || !backbufferColor || !backbufferDepthStencil || !pStream0)
		{
			return D3DERR_INVALIDCALL;
		}

		// Temporarily swap to new color buffer (keep original depth/stencil)
		ProxyInterface->SetRenderTarget(silhouetteSurface, backbufferDepthStencil);
		ProxyInterface->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

		CUSTOMVERTEX fullscreenQuad[] =
		{
			{0.0f, 0.0f, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255)},
			{0.0f, (float)BufferHeight, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255)},
			{(float)BufferWidth, 0.0f, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255)},
			{(float)BufferWidth, (float)BufferHeight, 0.0f, 1.0f, D3DCOLOR_ARGB(255, 255, 255, 255)}
		};

		// Bias coords to align correctly to screen space
		for (int i = 0; i < 4; i++)
		{
			fullscreenQuad[i].x -= 0.5f;
			fullscreenQuad[i].y -= 0.5f;
		}

		// Backup alpha blend and alpha test enable, disable both
		DWORD alphaBlend, alphaTest = 0;
		ProxyInterface->GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
		ProxyInterface->GetRenderState(D3DRS_ALPHATESTENABLE, &alphaTest);
		ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		ProxyInterface->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

		// Discard all pixels but James' silhouette
		DWORD stencilFunc = 0;
		ProxyInterface->GetRenderState(D3DRS_STENCILFUNC, &stencilFunc);
		ProxyInterface->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL);

		// Backup FVF, use pre-transformed vertices and diffuse color
		DWORD vshader = 0;
		ProxyInterface->GetVertexShader(&vshader);
		ProxyInterface->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

		// Backup cullmode, disable culling so we don't have to worry about winding order
		DWORD cullMode = 0;
		ProxyInterface->GetRenderState(D3DRS_CULLMODE, &cullMode);
		ProxyInterface->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

		// Draw James' silhouette to our texture
		ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, fullscreenQuad, 20);

		// Restore state
		ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
		ProxyInterface->SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest);
		ProxyInterface->SetStreamSource(0, pStream0, stream0Stride);
		ProxyInterface->SetRenderState(D3DRS_STENCILFUNC, stencilFunc);
		ProxyInterface->SetVertexShader(vshader);
		ProxyInterface->SetRenderState(D3DRS_CULLMODE, cullMode);

		// Bring back our original color buffer, clear stencil buffer like Xbox
		ProxyInterface->SetRenderTarget(backbufferColor, backbufferDepthStencil);
		ProxyInterface->Clear(0, NULL, D3DCLEAR_STENCIL, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 128);
	}

	return ProxyInterface->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
}

HRESULT m_IDirect3DDevice8::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
	Logging::LogDebug() << __FUNCTION__;

	// Draw Self/Soft Shadows
	DWORD stencilRef;
	GetRenderState(D3DRS_STENCILREF, &stencilRef);

	// Fix bowling cutscene fading
	if (WidescreenFix && SH2_CutsceneID && *SH2_CutsceneID == 0x19 && PrimitiveType == D3DPT_TRIANGLELIST && PrimitiveCount == 2 && VertexStreamZeroStride == 28 &&
		((CUSTOMVERTEX_UV_F*)pVertexStreamZeroData)[0].z == 0.01f && ((CUSTOMVERTEX_UV_F*)pVertexStreamZeroData)[1].z == 0.01f && ((CUSTOMVERTEX_UV_F*)pVertexStreamZeroData)[2].z == 0.01f)
	{
		for (const DWORD &x : { 1, 2, 4 })
		{
			FullScreenFadeout[x].x = (float)BufferWidth - 0.5f;
		}

		for (int x = 0; x < 6; x++)
		{
			FullScreenFadeout[x].y = ((CUSTOMVERTEX_UV_F*)pVertexStreamZeroData)[x].y;
			FullScreenFadeout[x].color = ((CUSTOMVERTEX_UV_F*)pVertexStreamZeroData)[x].color;
		}

		pVertexStreamZeroData = FullScreenFadeout;
	}

	if (stencilRef == 129)
	{
		if (EnableXboxShadows)
		{
			// Done drawing shadow volumes, toggle flag off
			shadowVolumeFlag = false;

			// DrawPrimitiveUP trashes vertex buffer stream 0, back it up
			IDirect3DVertexBuffer8 *pStream0;
			UINT stream0Stride = 0;
			if (SUCCEEDED(ProxyInterface->GetStreamSource(0, &pStream0, &stream0Stride)) && pStream0)
			{
				pStream0->Release();
			}

			// Error checking
			if (!silhouetteTexture || !pStream0)
			{
				return D3DERR_INVALIDCALL;
			}

			// Bind our texture of James' silhouette
			ProxyInterface->SetTexture(0, silhouetteTexture);

			CUSTOMVERTEX_UV fullscreenQuad[] =
			{
				{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
				{0.0f, (float)BufferHeight, 0.0f, 1.0f, 0.0f, 1.0f},
				{(float)BufferWidth, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
				{(float)BufferWidth, (float)BufferHeight, 0.0f, 1.0f, 1.0f, 1.0f}
			};

			// Bias coords to align correctly to screen space
			for (int i = 0; i < 4; i++)
			{
				fullscreenQuad[i].x -= 0.5f;
				fullscreenQuad[i].y -= 0.5f;
			}

			// Backup alpha blend and alpha test enable, disable alpha blend, enable alpha test
			DWORD alphaBlend, alphaTest = 0;
			ProxyInterface->GetRenderState(D3DRS_ALPHABLENDENABLE, &alphaBlend);
			ProxyInterface->GetRenderState(D3DRS_ALPHATESTENABLE, &alphaTest);
			ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			ProxyInterface->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);

			// Backup FVF, use pre-transformed vertices and texture coords
			DWORD vshader = 0;
			ProxyInterface->GetVertexShader(&vshader);
			ProxyInterface->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);

			// Backup cullmode, disable culling so we don't have to worry about winding order
			DWORD cullMode = 0;
			ProxyInterface->GetRenderState(D3DRS_CULLMODE, &cullMode);
			ProxyInterface->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

			// Backup stencil func, use D3DCMP_ALWAYS to stop stencil interfering with alpha test
			DWORD stencilFunc = 0;
			ProxyInterface->GetRenderState(D3DRS_STENCILFUNC, &stencilFunc);
			ProxyInterface->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);

			// Backup stencil pass, so that James' silhouette clears that portion of the stencil buffer
			DWORD stencilPass = 0;
			ProxyInterface->GetRenderState(D3DRS_STENCILPASS, &stencilPass);
			ProxyInterface->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_ZERO);

			// Backup color write value, we only want to touch the stencil buffer
			DWORD colorWrite = 0;
			ProxyInterface->GetRenderState(D3DRS_COLORWRITEENABLE, &colorWrite);
			ProxyInterface->SetRenderState(D3DRS_COLORWRITEENABLE, 0); // Comment me for debug fun

			// Use texture color and alpha
			DWORD colorArg1, alphaArg1 = 0;
			ProxyInterface->GetTextureStageState(0, D3DTSS_COLORARG1, &colorArg1);
			ProxyInterface->GetTextureStageState(0, D3DTSS_ALPHAARG1, &alphaArg1);
			ProxyInterface->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			ProxyInterface->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

			// Clear James' silhouette from the stencil buffer
			ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, fullscreenQuad, 24);

			// Backup alpha blend and alpha test enable, disable alpha blend, enable alpha test
			ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, alphaBlend);
			ProxyInterface->SetRenderState(D3DRS_ALPHATESTENABLE, alphaTest);
			ProxyInterface->SetStreamSource(0, pStream0, stream0Stride);
			ProxyInterface->SetVertexShader(vshader);
			ProxyInterface->SetRenderState(D3DRS_CULLMODE, cullMode);
			ProxyInterface->SetRenderState(D3DRS_STENCILFUNC, stencilFunc);
			ProxyInterface->SetRenderState(D3DRS_STENCILPASS, stencilPass);
			ProxyInterface->SetRenderState(D3DRS_COLORWRITEENABLE, colorWrite);
			ProxyInterface->SetTextureStageState(0, D3DTSS_COLORARG1, colorArg1);
			ProxyInterface->SetTextureStageState(0, D3DTSS_ALPHAARG1, alphaArg1);

			// Unbind silhouette texture, just to be safe
			ProxyInterface->SetTexture(0, NULL);
		}

		if (EnableSoftShadows)
		{
			return DrawSoftShadows();
		}
	}

	// Fix drawing line when using '0xFE' byte in '.mes' files
	if (FixDrawingTextLine)
	{
		if (PrimitiveType == D3DPT_LINELIST && PrimitiveCount == 3 && VertexStreamZeroStride == 20) {
			ProxyInterface->DrawPrimitiveUP(PrimitiveType, 1, pVertexStreamZeroData, VertexStreamZeroStride);
			PrimitiveType = D3DPT_TRIANGLESTRIP;
			PrimitiveCount = 2;
			pVertexStreamZeroData = (void *)((BYTE *)pVertexStreamZeroData + VertexStreamZeroStride * 2);
		}
	}

	return ProxyInterface->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}

HRESULT m_IDirect3DDevice8::BeginScene()
{
	Logging::LogDebug() << __FUNCTION__;

	// Skip frames in specific cutscenes to prevent flickering
	if (SkipSceneFlag == true)
	{
		return D3D_OK;
	}

	ClassReleaseFlag = false;

	// Enable Xbox shadows
	if (EnableSoftShadows)
	{
		EnableXboxShadows = !((SH2_RoomID && (*SH2_RoomID == 0x02 || *SH2_RoomID == 0x24 || *SH2_RoomID == 0x8F || *SH2_RoomID == 0x90)) ||
			(SH2_CutsceneID && *SH2_CutsceneID == 0x5A));
	}

	// Hotel Water Visual Fixes
	if (HotelWaterFix && SH2_RoomID)
	{
		UpdateHotelWater(SH2_RoomID);
	}

	// RPT Apartment Closet Cutscene Fix
	if (ClosetCutsceneFix && SH2_CutsceneID && SH2_CutsceneCameraPos)
	{
		UpdateClosetCutscene(SH2_CutsceneID, SH2_CutsceneCameraPos);
	}

	// RPT Hospital Elevator Stabbing Animation Fix
	if (HospitalChaseFix && SH2_RoomID && SH2_JamesPos)
	{
		UpdateHospitalChase(SH2_RoomID, SH2_JamesPos);
	}

	// Hang on Esc Fix
	if (FixHangOnEsc && SH2_RoomID)
	{
		UpdateHangOnEsc(SH2_RoomID);
	}

	// Fix infinite rumble in pause menu
	if (RestoreVibration && SH2_RoomID)
	{
		UpdateInfiniteRumble(SH2_RoomID);
	}

	// Fix draw distance in forest with chainsaw logs and Eddie boss meat cold room
	if (IncreaseDrawDistance && SH2_RoomID)
	{
		UpdateDynamicDrawDistance(SH2_RoomID);
	}

	// Lighting Transition fix
	if (LightingTransitionFix && SH2_CutsceneID)
	{
		UpdateLightingTransition(SH2_CutsceneID);
	}

	// Game save fix
	if (GameLoadFix && SH2_RoomID && SH2_JamesPos)
	{
		UpdateGameLoad(SH2_RoomID, SH2_JamesPos);
	}

	// Increase blood size
	if (IncreaseBlood && SH2_RoomID)
	{
		UpdateBloodSize(SH2_RoomID);
	}

	// Fix Fog volume in Hotel Room 312
	if (RestoreSpecialFX && SH2_RoomID)
	{
		UpdateHotelRoom312FogVolumeFix(SH2_RoomID);
	}

	// Disable shadow in specific cutscenes
	if (EnableSoftShadows && SH2_CutsceneID)
	{
		UpdateShadowCutscene(SH2_CutsceneID);
	}

	// Scale special FX based on resolution
	if (RestoreSpecialFX)
	{
		UpdateSpecialFXScale(BufferHeight);
	}

	// Scale the inner glow of the flashlight
	if (PS2FlashlightBrightness)
	{
		UpdateInnerFlashlightGlow(BufferHeight);
	}

	// Tree Lighting fix
	if (LightingFix && WidescreenFix && SH2_CutsceneID)
	{
		UpdateTreeColor(SH2_CutsceneID);
	}

	// Fix rotating Mannequin glitch
	if (WoodsideRoom205Fix && SH2_RoomID)
	{
		UpdateRotatingMannequin(SH2_RoomID);
	}

	return ProxyInterface->BeginScene();
}

HRESULT m_IDirect3DDevice8::GetStreamSource(THIS_ UINT StreamNumber, IDirect3DVertexBuffer8** ppStreamData, UINT* pStride)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->GetStreamSource(StreamNumber, ppStreamData, pStride);

	if (SUCCEEDED(hr) && ppStreamData)
	{
		*ppStreamData = ProxyAddressLookupTable->FindAddress<m_IDirect3DVertexBuffer8>(*ppStreamData);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::SetStreamSource(THIS_ UINT StreamNumber, IDirect3DVertexBuffer8* pStreamData, UINT Stride)
{
	Logging::LogDebug() << __FUNCTION__;

	if (pStreamData)
	{
		pStreamData = static_cast<m_IDirect3DVertexBuffer8 *>(pStreamData)->GetProxyInterface();
	}

	return ProxyInterface->SetStreamSource(StreamNumber, pStreamData, Stride);
}

HRESULT m_IDirect3DDevice8::GetBackBuffer(THIS_ UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->GetBackBuffer(iBackBuffer, Type, ppBackBuffer);

	if (SUCCEEDED(hr) && ppBackBuffer)
	{
		*ppBackBuffer = ProxyAddressLookupTable->FindAddress<m_IDirect3DSurface8>(*ppBackBuffer);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::GetDepthStencilSurface(IDirect3DSurface8 **ppZStencilSurface)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->GetDepthStencilSurface(ppZStencilSurface);

	if (SUCCEEDED(hr) && ppZStencilSurface)
	{
		*ppZStencilSurface = ProxyAddressLookupTable->FindAddress<m_IDirect3DSurface8>(*ppZStencilSurface);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::GetTexture(DWORD Stage, IDirect3DBaseTexture8 **ppTexture)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->GetTexture(Stage, ppTexture);

	if (SUCCEEDED(hr) && ppTexture && *ppTexture)
	{
		switch ((*ppTexture)->GetType())
		{
		case D3DRTYPE_TEXTURE:
			*ppTexture = ProxyAddressLookupTable->FindAddress<m_IDirect3DTexture8>(*ppTexture);
			break;
		case D3DRTYPE_VOLUMETEXTURE:
			*ppTexture = ProxyAddressLookupTable->FindAddress<m_IDirect3DVolumeTexture8>(*ppTexture);
			break;
		case D3DRTYPE_CUBETEXTURE:
			*ppTexture = ProxyAddressLookupTable->FindAddress<m_IDirect3DCubeTexture8>(*ppTexture);
			break;
		default:
			return D3DERR_INVALIDCALL;
		}
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD *pValue)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetTextureStageState(Stage, Type, pValue);
}

HRESULT m_IDirect3DDevice8::SetTexture(DWORD Stage, IDirect3DBaseTexture8 *pTexture)
{
	Logging::LogDebug() << __FUNCTION__;

	if (pTexture)
	{
		switch (pTexture->GetType())
		{
		case D3DRTYPE_TEXTURE:
			pTexture = static_cast<m_IDirect3DTexture8 *>(pTexture)->GetProxyInterface();
			break;
		case D3DRTYPE_VOLUMETEXTURE:
			pTexture = static_cast<m_IDirect3DVolumeTexture8 *>(pTexture)->GetProxyInterface();
			break;
		case D3DRTYPE_CUBETEXTURE:
			pTexture = static_cast<m_IDirect3DCubeTexture8 *>(pTexture)->GetProxyInterface();
			break;
		default:
			return D3DERR_INVALIDCALL;
		}
	}

	// Fix for the white shader issue
	if (WhiteShaderFix && Stage == 0 && !pTexture && BlankTexture)
	{
		pTexture = BlankTexture;
	}

	return ProxyInterface->SetTexture(Stage, pTexture);
}

HRESULT m_IDirect3DDevice8::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetTextureStageState(Stage, Type, Value);
}

HRESULT m_IDirect3DDevice8::UpdateTexture(IDirect3DBaseTexture8 *pSourceTexture, IDirect3DBaseTexture8 *pDestinationTexture)
{
	Logging::LogDebug() << __FUNCTION__;

	if (pSourceTexture)
	{
		switch (pSourceTexture->GetType())
		{
		case D3DRTYPE_TEXTURE:
			pSourceTexture = static_cast<m_IDirect3DTexture8 *>(pSourceTexture)->GetProxyInterface();
			break;
		case D3DRTYPE_VOLUMETEXTURE:
			pSourceTexture = static_cast<m_IDirect3DVolumeTexture8 *>(pSourceTexture)->GetProxyInterface();
			break;
		case D3DRTYPE_CUBETEXTURE:
			pSourceTexture = static_cast<m_IDirect3DCubeTexture8 *>(pSourceTexture)->GetProxyInterface();
			break;
		default:
			return D3DERR_INVALIDCALL;
		}
	}
	if (pDestinationTexture)
	{
		switch (pDestinationTexture->GetType())
		{
		case D3DRTYPE_TEXTURE:
			pDestinationTexture = static_cast<m_IDirect3DTexture8 *>(pDestinationTexture)->GetProxyInterface();
			break;
		case D3DRTYPE_VOLUMETEXTURE:
			pDestinationTexture = static_cast<m_IDirect3DVolumeTexture8 *>(pDestinationTexture)->GetProxyInterface();
			break;
		case D3DRTYPE_CUBETEXTURE:
			pDestinationTexture = static_cast<m_IDirect3DCubeTexture8 *>(pDestinationTexture)->GetProxyInterface();
			break;
		default:
			return D3DERR_INVALIDCALL;
		}
	}

	return ProxyInterface->UpdateTexture(pSourceTexture, pDestinationTexture);
}

HRESULT m_IDirect3DDevice8::ValidateDevice(DWORD *pNumPasses)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->ValidateDevice(pNumPasses);
}

HRESULT m_IDirect3DDevice8::GetClipPlane(DWORD Index, float *pPlane)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetClipPlane(Index, pPlane);
}

HRESULT m_IDirect3DDevice8::SetClipPlane(DWORD Index, CONST float *pPlane)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetClipPlane(Index, pPlane);
}

HRESULT m_IDirect3DDevice8::Clear(DWORD Count, CONST D3DRECT *pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
	Logging::LogDebug() << __FUNCTION__;

	// Change first Clear call to match Xbox version
	if (EnableXboxShadows && Flags == (D3DCLEAR_TARGET | D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER) && Color == D3DCOLOR_ARGB(124, 0, 0, 0))
	{
		return ProxyInterface->Clear(Count, pRects, Flags, Color, Z, 0);
	}

	return ProxyInterface->Clear(Count, pRects, Flags, Color, Z, Stencil);
}

HRESULT m_IDirect3DDevice8::GetViewport(D3DVIEWPORT8 *pViewport)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetViewport(pViewport);
}

HRESULT m_IDirect3DDevice8::SetViewport(CONST D3DVIEWPORT8 *pViewport)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetViewport(pViewport);
}

HRESULT m_IDirect3DDevice8::CreateVertexShader(THIS_ CONST DWORD* pDeclaration, CONST DWORD* pFunction, DWORD* pHandle, DWORD Usage)
{
	Logging::LogDebug() << __FUNCTION__;

	constexpr BYTE WallFixShaderCode[4][13] = {
		{ 0x0b, 0x00, 0x08, 0x80, 0x01, 0x00, 0xff, 0x80, 0x0e, 0x00, 0x00, 0xa0, 0x06 },		// Shader code:  max r11.w, r1.w, c14.x
		{ 0x0b, 0x00, 0x08, 0x80, 0x01, 0x00, 0xff, 0x80, 0x0e, 0x00, 0xff, 0xa0, 0x06 },		// Shader code:  max r11.w, r1.w, c14.w
		{ 0x0b, 0x00, 0x08, 0x80, 0x0a, 0x00, 0xff, 0x80, 0x0e, 0x00, 0x00, 0xa0, 0x06 },		// Shader code:  max r11.w, r10.w, c14.x
		{ 0x0b, 0x00, 0x08, 0x80, 0x0a, 0x00, 0xff, 0x80, 0x0e, 0x00, 0xff, 0xa0, 0x06 } };		// Shader code:  max r11.w, r10.w, c14.w

	if (FixMissingWallChunks && pFunction)
	{
		if (*pFunction >= D3DVS_VERSION(1, 0) && *pFunction <= D3DVS_VERSION(1, 1))
		{
			ReplaceMemoryBytes((void*)WallFixShaderCode[0], (void*)WallFixShaderCode[1], sizeof(WallFixShaderCode[0]), (DWORD)pFunction, 0x400, 1);
			ReplaceMemoryBytes((void*)WallFixShaderCode[2], (void*)WallFixShaderCode[3], sizeof(WallFixShaderCode[2]), (DWORD)pFunction, 0x400, 1);
		}
	}

	constexpr BYTE HalogenLightFixShaderCode[4][24] = {
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x07, 0x00, 0xe4, 0x90, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0, 0x05, 0x00, 0xe4, 0x90, 0x01 },			// Shader code:  mov oT0.xy, v7\n    mov oD0, v5
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x07, 0x00, 0xe4, 0x90, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0, 0x0e, 0x00, 0xff, 0xa0, 0x01 },			// Shader code:  mov oT0.xy, v7\n    mov oD0, c14.w
		{ 0xc0, 0x0e, 0x00, 0x00, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x01, 0x00, 0xe4, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 },			// Shader code:  mov oFog, c14.x\n   mov oPos, r1
		{ 0x80, 0x01, 0x00, 0xe4, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0, 0x01, 0x00, 0xe4, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 }};		// Shader code:  mov r1, r1\n        mov oPos, r1

	if (HalogenLightFix && pFunction)
	{
		if (*pFunction >= D3DVS_VERSION(1, 0) && *pFunction <= D3DVS_VERSION(1, 1))
		{
			ReplaceMemoryBytes((void*)HalogenLightFixShaderCode[0], (void*)HalogenLightFixShaderCode[1], sizeof(HalogenLightFixShaderCode[0]), (DWORD)pFunction, 0x400, 1);
			ReplaceMemoryBytes((void*)HalogenLightFixShaderCode[2], (void*)HalogenLightFixShaderCode[3], sizeof(HalogenLightFixShaderCode[2]), (DWORD)pFunction, 0x400, 1);
		}
	}

	return ProxyInterface->CreateVertexShader(pDeclaration, pFunction, pHandle, Usage);
}

HRESULT m_IDirect3DDevice8::GetVertexShader(THIS_ DWORD* pHandle)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetVertexShader(pHandle);
}

HRESULT m_IDirect3DDevice8::SetVertexShader(THIS_ DWORD Handle)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetVertexShader(Handle);
}

HRESULT m_IDirect3DDevice8::DeleteVertexShader(THIS_ DWORD Handle)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->DeleteVertexShader(Handle);
}

HRESULT m_IDirect3DDevice8::GetVertexShaderDeclaration(THIS_ DWORD Handle, void* pData, DWORD* pSizeOfData)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetVertexShaderDeclaration(Handle, pData, pSizeOfData);
}

HRESULT m_IDirect3DDevice8::GetVertexShaderFunction(THIS_ DWORD Handle, void* pData, DWORD* pSizeOfData)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetVertexShaderFunction(Handle, pData, pSizeOfData);
}

HRESULT m_IDirect3DDevice8::SetPixelShaderConstant(THIS_ DWORD Register, CONST void* pConstantData, DWORD ConstantCount)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetPixelShaderConstant(Register, pConstantData, ConstantCount);
}

HRESULT m_IDirect3DDevice8::GetPixelShaderConstant(THIS_ DWORD Register, void* pConstantData, DWORD ConstantCount)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetPixelShaderConstant(Register, pConstantData, ConstantCount);
}

HRESULT m_IDirect3DDevice8::SetVertexShaderConstant(THIS_ DWORD Register, CONST void* pConstantData, DWORD ConstantCount)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->SetVertexShaderConstant(Register, pConstantData, ConstantCount);
}

HRESULT m_IDirect3DDevice8::GetVertexShaderConstant(THIS_ DWORD Register, void* pConstantData, DWORD ConstantCount)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetVertexShaderConstant(Register, pConstantData, ConstantCount);
}

HRESULT m_IDirect3DDevice8::ResourceManagerDiscardBytes(THIS_ DWORD Bytes)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->ResourceManagerDiscardBytes(Bytes);
}

HRESULT m_IDirect3DDevice8::CreateImageSurface(THIS_ UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface)
{
	Logging::LogDebug() << __FUNCTION__;

	HRESULT hr = ProxyInterface->CreateImageSurface(Width, Height, Format, ppSurface);

	if (SUCCEEDED(hr) && ppSurface)
	{
		*ppSurface = ProxyAddressLookupTable->FindAddress<m_IDirect3DSurface8>(*ppSurface);
	}

	return hr;
}

HRESULT m_IDirect3DDevice8::CopyRects(THIS_ IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRectsArray, UINT cRects, IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPointsArray)
{
	Logging::LogDebug() << __FUNCTION__;

	if (pSourceSurface)
	{
		pSourceSurface = static_cast<m_IDirect3DSurface8 *>(pSourceSurface)->GetProxyInterface();
	}

	if (pDestinationSurface)
	{
		pDestinationSurface = static_cast<m_IDirect3DSurface8 *>(pDestinationSurface)->GetProxyInterface();
	}

	return ProxyInterface->CopyRects(pSourceSurface, pSourceRectsArray, cRects, pDestinationSurface, pDestPointsArray);
}

// Stretch source rect to destination rect
HRESULT m_IDirect3DDevice8::StretchRect(THIS_ IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface8* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
	Logging::LogDebug() << __FUNCTION__;

	UNREFERENCED_PARAMETER(Filter);

	// Check destination parameters
	if (!pSourceSurface || !pDestSurface)
	{
		return D3DERR_INVALIDCALL;
	}

	// Get surface desc
	D3DSURFACE_DESC SrcDesc, DestDesc;
	if (FAILED(pSourceSurface->GetDesc(&SrcDesc)))
	{
		return D3DERR_INVALIDCALL;
	}
	if (FAILED(pDestSurface->GetDesc(&DestDesc)))
	{
		return D3DERR_INVALIDCALL;
	}

	// Check rects
	RECT SrcRect, DestRect;
	if (!pSourceRect)
	{
		SrcRect.left = 0;
		SrcRect.top = 0;
		SrcRect.right = SrcDesc.Width;
		SrcRect.bottom = SrcDesc.Height;
	}
	else
	{
		memcpy(&SrcRect, pSourceRect, sizeof(RECT));
	}
	if (!pDestRect)
	{
		DestRect.left = 0;
		DestRect.top = 0;
		DestRect.right = DestDesc.Width;
		DestRect.bottom = DestDesc.Height;
	}
	else
	{
		memcpy(&DestRect, pDestRect, sizeof(RECT));
	}

	// Check if source and destination formats are the same
	if (SrcDesc.Format != DestDesc.Format)
	{
		return D3DERR_INVALIDCALL;
	}

	// Lock surface
	D3DLOCKED_RECT SrcLockRect, DestLockRect;
	if (FAILED(pSourceSurface->LockRect(&SrcLockRect, nullptr, D3DLOCK_NOSYSLOCK | D3DLOCK_READONLY)))
	{
		return D3DERR_INVALIDCALL;
	}
	if (FAILED(pDestSurface->LockRect(&DestLockRect, nullptr, D3DLOCK_NOSYSLOCK)))
	{
		pSourceSurface->UnlockRect();
		return D3DERR_INVALIDCALL;
	}

	// Get bit count
	DWORD ByteCount = SrcLockRect.Pitch / SrcDesc.Width;

	// Get width and height of rect
	LONG DestRectWidth = DestRect.right - DestRect.left;
	LONG DestRectHeight = DestRect.bottom - DestRect.top;
	LONG SrcRectWidth = SrcRect.right - SrcRect.left;
	LONG SrcRectHeight = SrcRect.bottom - SrcRect.top;

	// Get ratio
	float WidthRatio = (float)SrcRectWidth / (float)DestRectWidth;
	float HeightRatio = (float)SrcRectHeight / (float)DestRectHeight;

	// Copy memory using color key
	switch (ByteCount)
	{
	case 1: // 8-bit surfaces
	case 2: // 16-bit surfaces
	case 3: // 24-bit surfaces
	case 4: // 32-bit surfaces
	{
		for (LONG y = 0; y < DestRectHeight; y++)
		{
			DWORD StartDestLoc = ((y + DestRect.top) * DestLockRect.Pitch) + (DestRect.left * ByteCount);
			DWORD StartSrcLoc = ((((DWORD)((float)y * HeightRatio)) + SrcRect.top) * SrcLockRect.Pitch) + (SrcRect.left * ByteCount);

			for (LONG x = 0; x < DestRectWidth; x++)
			{
				memcpy((BYTE*)DestLockRect.pBits + StartDestLoc + x * ByteCount, (BYTE*)((BYTE*)SrcLockRect.pBits + StartSrcLoc + ((DWORD)((float)x * WidthRatio)) * ByteCount), ByteCount);
			}
		}
		break;
	}
	default: // Unsupported surface bit count
		pSourceSurface->UnlockRect();
		pDestSurface->UnlockRect();
		return D3DERR_INVALIDCALL;
	}

	// Unlock rect and return
	pSourceSurface->UnlockRect();
	pDestSurface->UnlockRect();
	return D3D_OK;
}

HRESULT m_IDirect3DDevice8::GetFrontBuffer(THIS_ IDirect3DSurface8* pDestSurface)
{
	Logging::LogDebug() << __FUNCTION__;

	if (pDestSurface)
	{
		pDestSurface = static_cast<m_IDirect3DSurface8 *>(pDestSurface)->GetProxyInterface();
	}

	if (EnableWndMode && DeviceWindow && BufferWidth && BufferHeight)
	{
		// Create new surface to hold data
		IDirect3DSurface8 *pSrcSurface = nullptr;
		int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
		int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
		if (FAILED(ProxyInterface->CreateImageSurface(ScreenWidth, ScreenHeight, D3DFMT_A8R8G8B8, &pSrcSurface)))
		{
			return D3DERR_INVALIDCALL;
		}

		// Get FrontBuffer data to new surface
		HRESULT hr = ProxyInterface->GetFrontBuffer(pSrcSurface);
		if (FAILED(hr))
		{
			pSrcSurface->Release();
			return hr;
		}

		// Get location of client window
		RECT RectSrc;
		if (!GetWindowRect(DeviceWindow, &RectSrc))
		{
			pSrcSurface->Release();
			return D3DERR_INVALIDCALL;
		}
		RECT rcClient;
		if (!GetClientRect(DeviceWindow, &rcClient))
		{
			pSrcSurface->Release();
			return D3DERR_INVALIDCALL;
		}
		int border_thickness = ((RectSrc.right - RectSrc.left) - rcClient.right) / 2;
		int top_border = (RectSrc.bottom - RectSrc.top) - rcClient.bottom - border_thickness;
		RectSrc.left += border_thickness;
		RectSrc.top += top_border;
		RectSrc.right = min(RectSrc.left + rcClient.right, ScreenWidth);
		RectSrc.bottom = min(RectSrc.top + rcClient.bottom, ScreenHeight);

		// Copy data to DestSurface
		hr = D3DERR_INVALIDCALL;
		if (BufferWidth == rcClient.right && BufferHeight == rcClient.bottom)
		{
			POINT PointDest = { 0, 0 };
			hr = ProxyInterface->CopyRects(pSrcSurface, &RectSrc, 1, pDestSurface, &PointDest);
		}

		// Try using StretchRect
		if (FAILED(hr))
		{
			RECT RectDest = { 0, 0, min(BufferWidth, ScreenWidth), min(BufferHeight, ScreenHeight) };
			hr = StretchRect(pSrcSurface, &RectSrc, pDestSurface, &RectDest, D3DTEXF_NONE);
		}

		// Release surface
		pSrcSurface->Release();
		return hr;
	}

	return ProxyInterface->GetFrontBuffer(pDestSurface);
}

HRESULT m_IDirect3DDevice8::GetInfo(THIS_ DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize)
{
	Logging::LogDebug() << __FUNCTION__;

	return ProxyInterface->GetInfo(DevInfoID, pDevInfoStruct, DevInfoStructSize);
}

HRESULT m_IDirect3DDevice8::DrawSoftShadows()
{
	DrawingShadowsFlag = true;
	SetShadowFading();

	// Variables for soft shadows
	const DWORD SHADOW_OPACITY = (ShadowMode == SHADOW_FADING_NONE) ? GetShadowOpacity() : (GetShadowOpacity() * ShadowFadingIntensity) / 100;
	const float SHADOW_DIVISOR = 2.0f * ((float)BufferHeight / 720.0f);
	const int BLUR_PASSES = 4;

	IDirect3DSurface8 *pBackBuffer = nullptr, *pStencilBuffer = nullptr;

	const float screenW = (float)BufferWidth;
	const float screenH = (float)BufferHeight;

	const float screenWs = (float)BufferWidth / SHADOW_DIVISOR;
	const float screenHs = (float)BufferHeight / SHADOW_DIVISOR;

	// Original geometry vanilla game uses
	CUSTOMVERTEX shadowRectDiffuse[] =
	{
		{0.0f, 0.0f, 0.0f, 1.0f, D3DCOLOR_ARGB(SHADOW_OPACITY, 0, 0, 0)},
		{0.0f, screenH, 0.0f, 1.0f, D3DCOLOR_ARGB(SHADOW_OPACITY, 0, 0, 0)},
		{screenW, 0.0f, 0.0f, 1.0f, D3DCOLOR_ARGB(SHADOW_OPACITY, 0, 0, 0)},
		{screenW, screenH, 0.0f, 1.0f, D3DCOLOR_ARGB(SHADOW_OPACITY, 0, 0, 0)}
	};

	// No need for diffuse color, used to render from texture, requires texture coords
	CUSTOMVERTEX_UV shadowRectUV_Small[] =
	{
		{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, screenHs, 0.0f, 1.0f, 0.0f, 1.0f},
		{screenWs, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
		{screenWs, screenHs, 0.0f, 1.0f, 1.0f, 1.0f}
	};

	// Bias coords to align correctly to screen space
	for (int i = 0; i < 4; i++)
	{
		shadowRectUV_Small[i].x -= 0.5f;
		shadowRectUV_Small[i].y -= 0.5f;
	}

	CUSTOMVERTEX_UV shadowRectUV[] =
	{
		{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, screenH, 0.0f, 1.0f, 0.0f, 1.0f},
		{screenW, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
		{screenW, screenH, 0.0f, 1.0f, 1.0f, 1.0f}
	};

	// Create our intermediate render targets/textures only once
	if (!pInTexture)
	{
		if (SUCCEEDED(ProxyInterface->CreateTexture((UINT)screenW, (UINT)screenH, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pInTexture)) && pInTexture)
		{
			pInTexture->GetSurfaceLevel(0, &pInSurface);
		}
	}

	if (!pShrunkTexture)
	{
		if (SUCCEEDED(ProxyInterface->CreateTexture((UINT)screenWs, (UINT)screenHs, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pShrunkTexture)) && pShrunkTexture)
		{
			pShrunkTexture->GetSurfaceLevel(0, &pShrunkSurface);
		}
	}

	if (!pOutTexture)
	{
		if (SUCCEEDED(ProxyInterface->CreateTexture((UINT)screenWs, (UINT)screenHs, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pOutTexture)) && pOutTexture)
		{
			pOutTexture->GetSurfaceLevel(0, &pOutSurface);
		}
	}

	if (!pInTexture || !pShrunkTexture || !pOutTexture ||
		!pInSurface || !pShrunkSurface || !pOutSurface)
	{
		return D3DERR_INVALIDCALL;
	}

	// Backup current state
	D3DSTATE state;
	BackupState(&state);

	// Textures will be scaled bi-linearly
	ProxyInterface->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	ProxyInterface->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);

	// Turn off alpha blending
	ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	ProxyInterface->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

	// Back up current render target (backbuffer) and stencil buffer
	if (SUCCEEDED(ProxyInterface->GetRenderTarget(&pBackBuffer)) && pBackBuffer)
	{
		pBackBuffer->Release();
	}
	if (SUCCEEDED(ProxyInterface->GetDepthStencilSurface(&pStencilBuffer)) && pStencilBuffer)
	{
		pStencilBuffer->Release();
	}

	// Swap to new render target, maintain old stencil buffer and draw shadows
	ProxyInterface->SetRenderTarget(pInSurface, pStencilBuffer);
	ProxyInterface->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

	HRESULT hr = ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, shadowRectDiffuse, 20);

	// Draw to a scaled down buffer first
	ProxyInterface->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
	ProxyInterface->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	ProxyInterface->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

	ProxyInterface->SetRenderTarget(pShrunkSurface, NULL);
	ProxyInterface->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	ProxyInterface->SetTexture(0, pInTexture);

	ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, shadowRectUV_Small, 24);

	// Create 4 full-screen quads, each offset a tiny amount diagonally in each direction
	CUSTOMVERTEX_UV blurUpLeft[] =
	{
		{    0.0f,     0.0f, 0.0f, 1.0f,    0.0f - (0.5f / screenWs), 0.0f - (0.5f / screenHs)},
		{    0.0f, screenHs, 0.0f, 1.0f,    0.0f - (0.5f / screenWs), 1.0f - (0.5f / screenHs)},
		{screenWs,     0.0f, 0.0f, 1.0f,    1.0f - (0.5f / screenWs), 0.0f - (0.5f / screenHs)},
		{screenWs, screenHs, 0.0f, 1.0f,    1.0f - (0.5f / screenWs), 1.0f - (0.5f / screenHs)}
	};

	CUSTOMVERTEX_UV blurDownLeft[] =
	{
		{    0.0f,     0.0f, 0.0f, 1.0f,    0.0f - (0.5f / screenWs), 0.0f + (0.5f / screenHs)},
		{    0.0f, screenHs, 0.0f, 1.0f,    0.0f - (0.5f / screenWs), 1.0f + (0.5f / screenHs)},
		{screenWs,     0.0f, 0.0f, 1.0f,    1.0f - (0.5f / screenWs), 0.0f + (0.5f / screenHs)},
		{screenWs, screenHs, 0.0f, 1.0f,    1.0f - (0.5f / screenWs), 1.0f + (0.5f / screenHs)}
	};

	CUSTOMVERTEX_UV blurUpRight[] =
	{
		{    0.0f,     0.0f, 0.0f, 1.0f,    0.0f + (0.5f / screenWs), 0.0f - (0.5f / screenHs)},
		{    0.0f, screenHs, 0.0f, 1.0f,    0.0f + (0.5f / screenWs), 1.0f - (0.5f / screenHs)},
		{screenWs,     0.0f, 0.0f, 1.0f,    1.0f + (0.5f / screenWs), 0.0f - (0.5f / screenHs)},
		{screenWs, screenHs, 0.0f, 1.0f,    1.0f + (0.5f / screenWs), 1.0f - (0.5f / screenHs)}
	};

	CUSTOMVERTEX_UV blurDownRight[] =
	{
		{    0.0f,     0.0f, 0.0f, 1.0f,    0.0f + (0.5f / screenWs), 0.0f + (0.5f / screenHs)},
		{    0.0f, screenHs, 0.0f, 1.0f,    0.0f + (0.5f / screenWs), 1.0f + (0.5f / screenHs)},
		{screenWs,     0.0f, 0.0f, 1.0f,    1.0f + (0.5f / screenWs), 0.0f + (0.5f / screenHs)},
		{screenWs, screenHs, 0.0f, 1.0f,    1.0f + (0.5f / screenWs), 1.0f + (0.5f / screenHs)}
	};

	// Bias coords to align correctly to screen space
	for (int j = 0; j < 4; j++)
	{
		blurUpLeft[j].x -= 0.5f;
		blurUpLeft[j].y -= 0.5f;

		blurDownLeft[j].x -= 0.5f;
		blurDownLeft[j].y -= 0.5f;

		blurUpRight[j].x -= 0.5f;
		blurUpRight[j].y -= 0.5f;

		blurDownRight[j].x -= 0.5f;
		blurDownRight[j].y -= 0.5f;

		// Undo biasing for blur step
		shadowRectUV_Small[j].x += 0.5f;
		shadowRectUV_Small[j].y += 0.5f;
	}

	// Perform fixed function blur
	for (int i = 0; i < BLUR_PASSES; i++)
	{
		ProxyInterface->SetRenderTarget(pOutSurface, NULL);
		ProxyInterface->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
		ProxyInterface->SetTexture(0, pShrunkTexture);

		// Should probably be combined into one
		ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, blurUpLeft, 24);
		ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, blurDownLeft, 24);
		ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, blurUpRight, 24);
		ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, blurDownRight, 24);

		ProxyInterface->SetRenderTarget(pShrunkSurface, NULL);
		ProxyInterface->SetTexture(0, pOutTexture);

		ProxyInterface->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
		ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, shadowRectUV_Small, 24);
	}

	// Return to backbuffer but without stencil buffer
	ProxyInterface->SetRenderTarget(pBackBuffer, NULL);
	ProxyInterface->SetTexture(0, pShrunkTexture);

	// Set up alpha-blending for final draw back to scene
	ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	ProxyInterface->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	ProxyInterface->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	// Bias coords to align correctly to screen space
	for (int i = 0; i < 4; i++)
	{
		shadowRectUV[i].x -= 0.5f;
		shadowRectUV[i].y -= 0.5f;
	}

	ProxyInterface->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, shadowRectUV, 24);

	// Return to original render target and stencil buffer
	ProxyInterface->SetRenderTarget(pBackBuffer, pStencilBuffer);

	RestoreState(&state);

	return hr;
}

void m_IDirect3DDevice8::BackupState(D3DSTATE *state)
{
	ProxyInterface->GetTextureStageState(0, D3DTSS_MAGFILTER, &state->magFilter);
	ProxyInterface->GetTextureStageState(0, D3DTSS_MINFILTER, &state->minFilter);
	ProxyInterface->GetTextureStageState(0, D3DTSS_COLORARG1, &state->colorArg1);
	ProxyInterface->GetTextureStageState(0, D3DTSS_ALPHAARG1, &state->alphaArg1);

	ProxyInterface->GetRenderState(D3DRS_ALPHABLENDENABLE, &state->alphaBlendEnable);	// Doesn't really need to be backed up
	ProxyInterface->GetRenderState(D3DRS_ALPHATESTENABLE, &state->alphaTestEnable);
	ProxyInterface->GetRenderState(D3DRS_SRCBLEND, &state->srcBlend);
	ProxyInterface->GetRenderState(D3DRS_DESTBLEND, &state->destBlend);

	if (SUCCEEDED(ProxyInterface->GetTexture(0, &state->stage0)) && state->stage0)		// Could use a later stage instead of backing up
	{
		state->stage0->Release();
	}

	ProxyInterface->GetVertexShader(&state->vertexShader);

	if (SUCCEEDED(ProxyInterface->GetStreamSource(0, &state->pStream0, &state->stream0Stride)) && state->pStream0)
	{
		state->pStream0->Release();
	}
}

void m_IDirect3DDevice8::RestoreState(D3DSTATE *state)
{
	ProxyInterface->SetTextureStageState(0, D3DTSS_MAGFILTER, state->magFilter);
	ProxyInterface->SetTextureStageState(0, D3DTSS_MINFILTER, state->minFilter);
	ProxyInterface->SetTextureStageState(0, D3DTSS_COLORARG1, state->colorArg1);
	ProxyInterface->SetTextureStageState(0, D3DTSS_ALPHAARG1, state->alphaArg1);

	ProxyInterface->SetRenderState(D3DRS_ALPHABLENDENABLE, state->alphaBlendEnable);
	ProxyInterface->SetRenderState(D3DRS_ALPHATESTENABLE, state->alphaTestEnable);
	if (state->pStream0)
	{
		ProxyInterface->SetStreamSource(0, state->pStream0, state->stream0Stride);
	}
	ProxyInterface->SetRenderState(D3DRS_SRCBLEND, state->srcBlend);
	ProxyInterface->SetRenderState(D3DRS_DESTBLEND, state->destBlend);

	if (state->stage0)
	{
		ProxyInterface->SetTexture(0, state->stage0);
	}

	ProxyInterface->SetVertexShader(state->vertexShader);
}

template <typename T>
void m_IDirect3DDevice8::ReleaseInterface(T **ppInterface, UINT ReleaseRefNum)
{
	if (ppInterface && *ppInterface && ReleaseRefNum)
	{
		UINT ref = 0;
		do {
			ref = (*ppInterface)->Release();
		} while (--ReleaseRefNum && ref);

		// Add Error: checking
		if (ref || ReleaseRefNum)
		{
			LOG_LIMIT(100, __FUNCTION__ << " Error: failed to release interface! " << ref);
		}

		*ppInterface = nullptr;
	}
}

// Get shadow opacity
DWORD m_IDirect3DDevice8::GetShadowOpacity()
{
	if (SH2_RoomID && SH2_ChapterID)
	{
		// Main scenario
		if (*SH2_ChapterID == 0x00)
		{
			switch (*SH2_RoomID)
			{
			case 0x0F:
				return 40;
			case 0xA2:
			case 0xBD:
				return 50;
			case 0x89:
				return 70;
			case 0x0B:
			case 0x21:
				return 110;
			case 0xBF:
				return 128;
			}
		}
		// Born From a Wish chapter
		else if (*SH2_ChapterID == 0x01)
		{
			switch (*SH2_RoomID)
			{
			case 0x27:
				return 30;
			case 0xC2:
				return 60;
			case 0x0C:
			case 0x0D:
			case 0x20:
			case 0x21:
			case 0x25:
			case 0x26:
			case 0xC0:
			case 0xC1:
			case 0xC3:
			case 0xC4:
			case 0xC5:
			case 0xC6:
			case 0xC8:
			case 0xC9:
			case 0xCA:
			case 0xCB:
			case 0xCC:
			case 0xCD:
			case 0xCE:
			case 0xCF:
			case 0xD0:
			case 0xD1:
			case 0xD2:
			case 0xD5:
				return 90;
			case 0xC7:
				return 128;
			}
		}
	}

	// Default opacity
	return 170;
}

// Get shadow fading intensity
DWORD m_IDirect3DDevice8::GetShadowIntensity()
{
	return (!SH2_FlashlightRed || !SH2_FlashlightGreen || !SH2_FlashlightBlue) ? 100 :
		(DWORD)(((*SH2_FlashlightRed + *SH2_FlashlightGreen + *SH2_FlashlightBlue) * 33.3333333f) / 7.0f);
}

// Check if shadow fading needs to be done and set the shadow intensity
void m_IDirect3DDevice8::SetShadowFading()
{
	if (!SH2_FlashlightBeam || !SH2_FlashlightSwitch)
	{
		return;
	}

	// Check room ID to see if shadow fading should be enabled
	bool EnableShadowFading = true;
	for (const DWORD &Room : { 0x89, 0xA2, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBD })
	{
		if (SH2_RoomID && *SH2_RoomID == Room)
		{
			EnableShadowFading = false;
			break;
		}
	}

	// Fade shadows in after turning flashlight on
	if (EnableShadowFading && (LastFlashlightSwitch == 0x00 || ShadowMode != SHADOW_FADING_NONE) && *SH2_FlashlightSwitch == 0x01)
	{
		if (ShadowMode != SHADOW_FADING_IN)
		{
			ShadowFadingCounter = 0;
		}
		ShadowMode = SHADOW_FADING_IN;
		ShadowFadingIntensity = GetShadowIntensity();	// Intesity is increased by around 3 each frame
		if (ShadowFadingIntensity == 100)				// Exit once intensity reaches 100
		{
			LastFlashlightSwitch = *SH2_FlashlightSwitch;
			ShadowMode = SHADOW_FADING_NONE;
		}
	}
	// Fade shadows out after turning flashlight off
	else if (EnableShadowFading && *SH2_FlashlightSwitch == 0x00 && *SH2_FlashlightBeam == 0x01)
	{
		if (ShadowMode != SHADOW_FADING_OUT)
		{
			ShadowFadingCounter = 0;
		}
		ShadowMode = SHADOW_FADING_OUT;
		ShadowFadingIntensity = GetShadowIntensity();	// Intesity is decreased by around 15 each frame
	}
	// Refade in other shadows after turning flashlight off
	else if (EnableShadowFading && ((SH2_FlashlightSwitch && *SH2_FlashlightSwitch != LastFlashlightSwitch) || ShadowMode == SHADOW_FADING_OUT || ShadowMode == SHADOW_REFADING))
	{
		if (ShadowMode != SHADOW_REFADING)
		{
			ShadowFadingCounter = 0;
			ShadowFadingIntensity = 0;
		}
		ShadowMode = SHADOW_REFADING;
		ShadowFadingIntensity = min(100, ShadowFadingCounter / 2);	// Intesity is increased by 1 every other frame
		if (ShadowFadingIntensity == 100)							// Exit once intensity reaches 100
		{
			LastFlashlightSwitch = *SH2_FlashlightSwitch;
			ShadowMode = SHADOW_FADING_NONE;
		}
	}
	// No shadow fading effects
	else
	{
		ShadowMode = SHADOW_FADING_NONE;
		LastFlashlightSwitch = *SH2_FlashlightSwitch;
		ShadowFadingIntensity = LastFlashlightSwitch * 100;
	}
}
