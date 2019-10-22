#pragma once

#include <DirectXMath.h>

namespace alexis
{
	class Camera
	{
	public:
		Camera();
		~Camera();

		DirectX::XMMATRIX GetViewMatrix() const;
		DirectX::XMMATRIX GetInvViewMatrix() const;

		DirectX::XMMATRIX GetProjMatrix() const;
		DirectX::XMMATRIX GetInvProjMatrix() const;

		void XM_CALLCONV LookAt(DirectX::FXMVECTOR eyePos, DirectX::FXMVECTOR targetPos, DirectX::FXMVECTOR up);

		void SetProjectionParams(float verticalFov, float aspectRatio, float zNear, float zFar);

		void SetVerticalFov(float verticalFov);
		float GetVerticalFov() const;

		void XM_CALLCONV SetTranslation(DirectX::FXMVECTOR translation);
		DirectX::FXMVECTOR GetTranslation() const;

		void XM_CALLCONV SetRotation(DirectX::FXMVECTOR rotation);
		DirectX::FXMVECTOR GetRotation() const;

		void XM_CALLCONV Translate(DirectX::FXMVECTOR translation, bool isLocal = false);
		void Rotate(DirectX::FXMVECTOR rotationQuat);

	private:
		void UpdateViewMatrix() const;
		void UpdateInvViewMatrix() const;

		void UpdateProjMatrix() const;
		void UpdateInvProjMatrix() const;

		// Must be aligned for SSE calls
		__declspec(align(16)) struct AlignedCameraData
		{
			DirectX::XMVECTOR m_translation;
			DirectX::XMVECTOR m_rotation; // This is quaternion

			DirectX::XMMATRIX m_viewMatrix;
			DirectX::XMMATRIX m_invViewMatrix;

			DirectX::XMMATRIX m_projMatrix;
			DirectX::XMMATRIX m_invProjMatrix;
		};

		AlignedCameraData* m_cameraData{ nullptr };

		// Projection params
		float m_verticalFov{ 45.0f };
		float m_aspectRatio{ 1.0f };
		float m_near{ 0.1f };
		float m_far{ 100.0f };

		// Dirtiness of the matrices after transform applied
		mutable bool m_isViewDirty{ true };
		mutable bool m_isInvViewDirty{ true };
		mutable bool m_isProjDirty{ true };
		mutable bool m_isInvProjDirty{ true };
	};

}