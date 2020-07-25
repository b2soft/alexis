#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <tuple>
#include <numbers>
#include <Render/RenderTarget.h>

namespace std
{
	namespace numbers
	{
		inline constexpr float pi_2 = pi / 2.0f;
	}
}

namespace alexis
{
	namespace utils
	{
		inline DXGI_FORMAT GetFormatForSrv(DXGI_FORMAT inFormat)
		{
			switch (inFormat)
			{
			case DXGI_FORMAT_R24G8_TYPELESS:
			{
				return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			}
			default:
				return inFormat;
			}
		}

		inline DXGI_FORMAT GetFormatForDsv(DXGI_FORMAT inFormat)
		{
			switch (inFormat)
			{
			case DXGI_FORMAT_R24G8_TYPELESS:
			{
				return DXGI_FORMAT_D24_UNORM_S8_UINT;
			}
			default:
				return inFormat;
			}
		}

		inline std::tuple<std::wstring, RenderTarget::Slot, bool> ParseRTName(const std::wstring& texturePath)
		{
			if (texturePath[0] == L'$')
			{
				auto indexPos = texturePath.find(L'#');
				auto rtName = texturePath.substr(1, indexPos - 1);

				int index = 0;

				if (indexPos != std::wstring::npos)
				{
					auto indexStr = texturePath.substr(indexPos + 1);
					if (indexStr == L"Depth")
					{
						index = RenderTarget::Slot::DepthStencil;
					}
					else
					{
						index = _wtoi(indexStr.c_str());
					}
				}

				return { rtName, static_cast<RenderTarget::Slot>(index), true };
			}
			else
			{
				return { texturePath, RenderTarget::Slot::NumAttachmentPoints, false };
			}
		}

		inline DirectX::XMFLOAT3 GetPitchYawRollFromQuaternion(DirectX::XMVECTOR rotation)
		{
			DirectX::XMFLOAT4 rotQ{};
			DirectX::XMStoreFloat4(&rotQ, rotation);

			float pitch = 0;
			float yaw = 0;
			float roll = 0;

			float sp = -2.0f * (rotQ.y * rotQ.z - rotQ.w * rotQ.x);

			if (std::abs(sp) > 0.9999f)
			{
				pitch = std::numbers::pi_2 * sp; // pi/2

				yaw = std::atan2(-rotQ.x * rotQ.z + rotQ.w * rotQ.y, 0.5f - rotQ.y * rotQ.y - rotQ.z * rotQ.z);
				roll = 0.f;
			}
			else
			{
				pitch = std::asin(sp);
				yaw = std::atan2(rotQ.x * rotQ.z + rotQ.w * rotQ.y, 0.5f - rotQ.x * rotQ.x - rotQ.y * rotQ.y);
				roll = std::atan2(rotQ.x * rotQ.y + rotQ.w * rotQ.z, 0.5f - rotQ.x * rotQ.x - rotQ.z * rotQ.z);
			}

			return { pitch, yaw, roll };
		}
	}
}

