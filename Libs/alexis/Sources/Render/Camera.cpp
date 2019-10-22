#include <Precompiled.h>

#include "Camera.h"

#include <CoreHelpers.h>

namespace alexis
{

	Camera::Camera()
	{
		m_cameraData = static_cast<AlignedCameraData*>(_aligned_malloc(sizeof(AlignedCameraData), 16));
		m_cameraData->m_translation = XMVectorZero();
		m_cameraData->m_rotation = XMQuaternionIdentity();
	}

	Camera::~Camera()
	{
		_aligned_free(m_cameraData);
	}

	DirectX::XMMATRIX Camera::GetViewMatrix() const
	{
		if (m_isViewDirty)
		{
			UpdateViewMatrix();
		}

		return m_cameraData->m_viewMatrix;
	}

	DirectX::XMMATRIX Camera::GetInvViewMatrix() const
	{
		if (m_isInvViewDirty)
		{
			m_cameraData->m_invViewMatrix = XMMatrixInverse(nullptr, m_cameraData->m_viewMatrix);
			m_isInvViewDirty = false;
		}

		return m_cameraData->m_invViewMatrix;
	}

	DirectX::XMMATRIX Camera::GetProjMatrix() const
	{
		if (m_isProjDirty)
		{
			UpdateProjMatrix();
		}

		return m_cameraData->m_projMatrix;
	}

	DirectX::XMMATRIX Camera::GetInvProjMatrix() const
	{
		if (m_isInvProjDirty)
		{
			UpdateInvProjMatrix();
		}

		return m_cameraData->m_invProjMatrix;
	}

	void XM_CALLCONV Camera::LookAt(DirectX::FXMVECTOR eyePos, DirectX::FXMVECTOR targetPos, DirectX::FXMVECTOR up)
	{
		m_cameraData->m_viewMatrix = XMMatrixLookAtLH(eyePos, targetPos, up);

		m_cameraData->m_translation = eyePos;
		m_cameraData->m_rotation = XMQuaternionRotationMatrix(XMMatrixTranspose(m_cameraData->m_viewMatrix));

		m_isInvViewDirty = true;
		m_isViewDirty = false;
	}

	void Camera::SetProjectionParams(float verticalFov, float aspectRatio, float zNear, float zFar)
	{
		m_verticalFov = verticalFov;
		m_aspectRatio = aspectRatio;
		m_near = zNear;
		m_far = zFar;

		m_isProjDirty = true;
		m_isInvProjDirty = true;
	}

	void Camera::SetVerticalFov(float verticalFov)
	{
		if (!IsEqual(m_verticalFov, verticalFov))
		{
			m_verticalFov = verticalFov;
			m_isProjDirty = true;
			m_isInvProjDirty = true;
		}
	}

	float Camera::GetVerticalFov() const
	{
		return m_verticalFov;
	}

	void XM_CALLCONV Camera::SetTranslation(DirectX::FXMVECTOR translation)
	{
		m_cameraData->m_translation = translation;
		m_isViewDirty = true;
	}

	DirectX::FXMVECTOR Camera::GetTranslation() const
	{
		return m_cameraData->m_translation;
	}

	void XM_CALLCONV Camera::SetRotation(DirectX::FXMVECTOR rotation)
	{
		m_cameraData->m_rotation = rotation;
	}

	DirectX::FXMVECTOR Camera::GetRotation() const
	{
		return m_cameraData->m_rotation;
	}

	void XM_CALLCONV Camera::Translate(DirectX::FXMVECTOR translation, bool isLocal /*= false*/)
	{
		if (isLocal)
		{
			m_cameraData->m_translation += XMVector3Rotate(translation, m_cameraData->m_rotation);
		}
		else
		{
			m_cameraData->m_translation += translation;
		}

		m_cameraData->m_translation = XMVectorSetW(m_cameraData->m_translation, 1.0f);

		m_isViewDirty = true;
		m_isInvViewDirty = true;
	}

	void Camera::Rotate(DirectX::FXMVECTOR rotationQuat)
	{
		m_cameraData->m_rotation = XMQuaternionMultiply(m_cameraData->m_rotation, rotationQuat);

		m_isViewDirty = true;
		m_isInvViewDirty = true;
	}

	void Camera::UpdateViewMatrix() const
	{
		XMMATRIX translationMatrix = XMMatrixTranslationFromVector(-(m_cameraData->m_translation));
		XMMATRIX rotationMatrix = XMMatrixTranspose(XMMatrixRotationQuaternion(m_cameraData->m_rotation));

		m_cameraData->m_viewMatrix = translationMatrix * rotationMatrix;

		m_isInvViewDirty = true;
		m_isViewDirty = false;
	}

	void Camera::UpdateInvViewMatrix() const
	{
		if (m_isViewDirty)
		{
			UpdateViewMatrix();
		}

		m_cameraData->m_invViewMatrix = XMMatrixInverse(nullptr, m_cameraData->m_viewMatrix);
		m_isInvViewDirty = false;
	}

	void Camera::UpdateProjMatrix() const
	{
		m_cameraData->m_projMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_verticalFov), m_aspectRatio, m_near, m_far);

		m_isProjDirty = false;
		m_isInvProjDirty = true;
	}

	void Camera::UpdateInvProjMatrix() const
	{
		if (m_isProjDirty)
		{
			UpdateProjMatrix();
		}

		m_cameraData->m_invProjMatrix = XMMatrixInverse(nullptr, m_cameraData->m_projMatrix);
		m_isInvProjDirty = false;
	}
}

