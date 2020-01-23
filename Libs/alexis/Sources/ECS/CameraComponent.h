#pragma once

#include <DirectXMath.h>

namespace alexis
{
	namespace ecs
	{
		struct CameraComponent
		{
			CameraComponent()
			{
				CameraData = static_cast<AlignedCameraData*>(_aligned_malloc(sizeof(AlignedCameraData), 16));
			}

			CameraComponent(float fov, float aspectRatio, float nearZ, float farZ, bool isOrtho) :
				Fov(fov),
				AspectRatio(aspectRatio),
				NearZ(nearZ),
				FarZ(farZ),
				IsOrtho(isOrtho)
			{
				CameraData = static_cast<AlignedCameraData*>(_aligned_malloc(sizeof(AlignedCameraData), 16));
			}

			CameraComponent(const CameraComponent& other) :
				Fov(other.Fov),
				AspectRatio(other.AspectRatio),
				NearZ(NearZ),
				FarZ(FarZ),
				IsOrtho(other.IsOrtho)
			{
				CameraData = static_cast<AlignedCameraData*>(_aligned_malloc(sizeof(AlignedCameraData), 16));
				memcpy_s(CameraData, sizeof(AlignedCameraData), other.CameraData, sizeof(AlignedCameraData));
			}

			CameraComponent& operator=(const CameraComponent& other)
			{
				Fov = other.Fov;
				AspectRatio = other.AspectRatio;
				NearZ = other.NearZ;
				FarZ = other.FarZ;
				IsOrtho = other.IsOrtho;

				CameraData = static_cast<AlignedCameraData*>(_aligned_malloc(sizeof(AlignedCameraData), 16));
				memcpy_s(CameraData, sizeof(AlignedCameraData), other.CameraData, sizeof(AlignedCameraData));

				return *this;
			}

			~CameraComponent()
			{
				_aligned_free(CameraData);
				CameraData = nullptr;
			}

			float Fov{ 45.0f };
			float AspectRatio{ 1.0f };
			float NearZ{ 0.01f };
			float FarZ{ 100.0f };
			bool IsOrtho{ false };

			__declspec(align(16)) struct AlignedCameraData
			{
				DirectX::XMMATRIX View;
				DirectX::XMMATRIX InvView;

				DirectX::XMMATRIX Proj;
				DirectX::XMMATRIX InvProj;
			};

			AlignedCameraData* CameraData{ nullptr };

			bool IsViewDirty{ true };
			bool IsInvViewDirty{ true };
			bool IsProjDirty{ true };
			bool IsInvProjDirty{ true };
		};
	}
}