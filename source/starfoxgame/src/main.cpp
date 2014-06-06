#include "gs/System/System.h"
#include "gs/Rendering/GraphicsEngine.h"
#include "gs/System/FrameTimer.h"
#include "gs/Platform/GL/GLHeaders.h"
#include "gs/Base/string_helpers.h"
#include "gs/Input/KeyboardMgr.h"

#include "gs/Math/Vector3.h"
#include "gs/Math/Matrix43.h"
#include "gs/Math/Angle.h"
#include "gs/Math/EulerAngles.h"
#include "gs/Math/Quaternion.h"
#include "gs/Math/MathUtil.h"
#include "gs/Scene/SceneNode.h"

#include "FbxLoader.h"
#include "StaticMesh.h"

#include <map>

//const float SCREEN_WIDTH = 1680.f;
//const float SCREEN_HEIGHT = 1050.f;
const float SCREEN_WIDTH = 1280.f;
const float SCREEN_HEIGHT = 960.f;
//const float SCREEN_WIDTH = 1024.f;
//const float SCREEN_HEIGHT = 768;
//const float SCREEN_WIDTH = 800.f;
//const float SCREEN_HEIGHT = 600;

const VKEY vkeyPause = 'P';
const VKEY vkeyStepFrame = VK_OEM_4; // [{
const VKEY vkeyStepFrameRepeatModifier = VK_OEM_6; // ]}
const VKEY vkeyTimeScaleInc = VK_OEM_PLUS; // +=
const VKEY vkeyTimeScaleDec = VK_OEM_MINUS; // -_
const VKEY vkeyTimeScaleReset = '0';
const VKEY vkeyCamUp = VK_UP;
const VKEY vkeyCamDown = VK_DOWN;
const VKEY vkeyCamLeft = VK_LEFT;
const VKEY vkeyCamRight = VK_RIGHT;
const VKEY vkeyCamZoomIn = VK_PRIOR; // Page up
const VKEY vkeyCamZoomOut = VK_NEXT; // Page down
const VKEY vkeyCamReset = VK_HOME;
const VKEY vkeyShipLeft = 'A';
const VKEY vkeyShipRight = 'D';
const VKEY vkeyShipUp = 'W';
const VKEY vkeyShipDown = 'S';

static bool g_drawNormals = false;
static bool g_drawSockets = false;
static float g_normalScale = 10.0f;
static bool g_renderSceneGraph = false;

FrameTimer& GetFrameTimer()
{
	static FrameTimer frameTimer;
	return frameTimer;
}

class DebugDrawManager
{
public:
	struct Line { Vector3 v1, v2; Color4F color; };

	void AddLine(const Line& line) { m_lines.push_back(line); }

	void Clear()
	{
		m_lines.clear();
	}

	void Render()
	{
		glPushAttrib(GL_LIGHTING_BIT|GL_TEXTURE_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		
		
		glBegin(GL_LINES);
		for (const Line& line : m_lines)
		{
			glColor4fv(line.color.v);
			glVertex3fv(line.v1.v);
			glVertex3fv(line.v2.v);
		}
		glEnd();
		
		
		glPopAttrib();

		Clear();
	}

private:
	std::vector<Line> m_lines;
};

DebugDrawManager g_debugDrawManager;

void DebugDrawLine(const Vector3& v1, const Vector3& v2, const Color4F& color = Color4F::White())
{
	DebugDrawManager::Line line = {v1, v2, color};
	g_debugDrawManager.AddLine( line );
}

void DebugDrawAxes(const Matrix43& m, float32 scale = 1.f)
{
	DebugDrawLine(m.trans, m.trans + m.axisX * scale, Color4F::Red());
	DebugDrawLine(m.trans, m.trans + m.axisY * scale, Color4F::Green());
	DebugDrawLine(m.trans, m.trans + m.axisZ * scale, Color4F::Blue());
}

void DrawStaticMesh(const gfx::StaticMesh& staticMesh, const Matrix43& mMeshToWorld)
{
	GLUtil::PushAndMultMatrix(mMeshToWorld);

	for (const auto& subMesh : staticMesh.m_subMeshes)
	{
		bool disabledTexturing = false;

		//glBindTexture(GL_TEXTURE_2D, subMesh.m_material.m_textureId);
		if (subMesh.m_materialIndex != gfx::StaticMesh::INVALID_MATERIAL_INDEX)
		{
			const gfx::Material& material = staticMesh.m_materials[subMesh.m_materialIndex];

			glMaterialfv(GL_FRONT, GL_AMBIENT, material.m_ambient.v);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, material.m_diffuse.v);
			glMaterialfv(GL_FRONT, GL_SPECULAR, material.m_specular.v);
			glMaterialfv(GL_FRONT, GL_EMISSION, material.m_emmissive.v);
			glMaterialf(GL_FRONT, GL_SHININESS, material.m_shininess);

			if (material.m_textureId == INVALID_TEXTURE_ID)
			{
				disabledTexturing = true;
				glDisable(GL_TEXTURE_2D);
			}
			else
			{			
				GLUtil::SelectTexture(material.m_textureId);
			}
		}

		glBegin(GL_TRIANGLES);
		for (const auto& vertex : subMesh.m_vertices)
		{
			glColor4fv(vertex.color.v);
			glNormal3fv((vertex.normal /** mMeshToWorld*/).v);
			glTexCoord2fv(vertex.textureCoords.v);
			glVertex3fv((vertex.position /** mMeshToWorld*/).v);
		}
		glEnd();

		if (disabledTexturing)
			glEnable(GL_TEXTURE_2D);

		// Draw normals
		if (g_drawNormals)
		{
			glPushAttrib(GL_LIGHTING_BIT|GL_TEXTURE_BIT);
			glDisable(GL_LIGHTING);
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINES);
			glColor4f(1.f, 1.f, 1.f, 1.f);
			for (const auto& vertex : subMesh.m_vertices)
			{
				glVertex3fv(vertex.position.v);
				glVertex3f(vertex.position.x + vertex.normal.x * g_normalScale, vertex.position.y + vertex.normal.y * g_normalScale, vertex.position.z + vertex.normal.z * g_normalScale);
			}
			glEnd();
			glPopAttrib();
		}
	}

	if (g_drawSockets)
	{
		for (auto& socket : staticMesh.m_sockets)
		{
			static float32 scale = 10.f;
			DebugDrawAxes(socket.m_matrix, scale);
		}
	}

	glPopMatrix();
}

class OrbitTargetCameraComponent : public SceneNodeComponent
{
public:
	OrbitTargetCameraComponent()
		: m_angles(EulerAngles::Identity())
		, m_offset(250.f)
	{
	}

	void SetTarget(const SceneNodeWeakPtr& target)
	{
		m_pwTarget = target;
	}

	virtual void Update(float32 deltaTime)
	{
		auto pTarget = m_pwTarget.lock();
		if (!pTarget)
			return;

		KeyboardMgr& kbMgr = KeyboardMgr::Instance();		
		static float32 cameraTurnRate = kPiOver2;
		static float32 zoomRate = 100.f;

		if (kbMgr[vkeyCamUp].IsDown() || kbMgr[vkeyCamDown].IsDown())
		{
			const float32 turnAngle = (kbMgr[vkeyCamUp].IsDown()? -1.f : 1.f) * (cameraTurnRate * deltaTime);
			m_angles.pitch += turnAngle;
		}
		if (kbMgr[vkeyCamLeft].IsDown() || kbMgr[vkeyCamRight].IsDown())
		{
			const float32 turnAngle = (kbMgr[vkeyCamLeft].IsDown()? -1.f : 1.f) * (cameraTurnRate * deltaTime);
			m_angles.yaw += turnAngle;
		}
		if (kbMgr[vkeyCamZoomIn].IsDown() || kbMgr[vkeyCamZoomOut].IsDown())
		{
			const float32 delta = (kbMgr[vkeyCamZoomIn].IsDown()? -1.f : 1.f) * (zoomRate * deltaTime);
			m_offset = MathEx::Max(0.f, m_offset + delta);
		}
		if (kbMgr[vkeyCamReset].JustPressed())
		{
			m_angles = EulerAngles::Zero();
			m_offset = 250.f;
		}
		//m_angles.Canonize();

		auto& mCamera = GetSceneNode()->ModifyLocalToParent();
		mCamera.SetFromEulerAngles(m_angles);
		mCamera.trans = pTarget->GetLocalToWorld().trans + DirectionVector(Vector3(0,0,-m_offset)) * mCamera;
	}

private:
	SceneNodeWeakPtr m_pwTarget;
	float32 m_offset;
	EulerAngles m_angles;
};

class FollowShipCameraComponent : public SceneNodeComponent
{
public:
	FollowShipCameraComponent()
		: m_offset(350.f)
	{
	}

	void SetTarget(const SceneNodeWeakPtr& pwTarget)
	{
		m_pwTarget = pwTarget;
		UpdateTransform(0.f, false);
	}

	virtual void Update(float32 deltaTime)
	{
		UpdateTransform(deltaTime, true);
	}

private:
	void UpdateTransform(float32 deltaTime, bool damp)
	{
		auto pTarget = m_pwTarget.lock();
		if (!pTarget)
			return;

		auto psAnchor = pTarget->GetParent();
		const Matrix43& mAnchorWorld = psAnchor->GetLocalToWorld();
		const Matrix43& mShipWorld = pTarget->GetLocalToWorld();		
		Matrix43& mCamWorld = GetSceneNode()->ModifyLocalToWorld();

		// First thing, we want the camera to be a fixed distance behind the ship. We do
		// this in anchor space since the ship yaws.
		Matrix43 mInvAnchorWorld; mInvAnchorWorld.SetInverseFromSRT(mAnchorWorld);
		Vector3 vCamPosInAnchorSpace = PositionVector(mCamWorld.trans) * mInvAnchorWorld;
		vCamPosInAnchorSpace.z = -m_offset;
		mCamWorld.trans = PositionVector(vCamPosInAnchorSpace) * mAnchorWorld;

		// Now approach the camera in the xy plane to be behind the ship
		TWEAKABLE float32 timeToTarget = 0.5f;
		Vector3 vCamDestPos = mShipWorld.trans - mAnchorWorld.axisZ * m_offset;
		Vector3 vCamFinalPos = damp? ApproachDamped(mCamWorld.trans, vCamDestPos, deltaTime, 0.99f, timeToTarget, 1.f) : vCamDestPos;

		//TWEAKABLE float32 approachSpeed = 100.f;
		//Vector3 vCamFinalPos = damp? ApproachLinear(mCamWorld.trans, vCamDestPos, deltaTime, approachSpeed) : vCamDestPos;


		//TWEAKABLE float32 maxAllowedDistX = 50.f;
		//TWEAKABLE float32 maxAllowedDistY = 50.f;
		//Vector3 vDelta = vCamDestPos - mCamWorld.trans;
		//assert(vDelta.z == 0.f);
		//Vector3 vCamFinalPos = mCamWorld.trans;
		//const bool outsideAllowedDistX = MathEx::Abs(vDelta.x) > maxAllowedDistX;
		//const bool outsideAllowedDistY = MathEx::Abs(vDelta.y) > maxAllowedDistY;
		//if (outsideAllowedDistX || outsideAllowedDistY)
		//{
		//	Vector3 vDeltaNorm = Normalize(vDelta);

		//	vCamDestPos = mCamWorld.trans;

		//	if (outsideAllowedDistX)
		//	{
		//		vCamDestPos.x = mCamWorld.trans.x + (vDelta.x - vDeltaNorm.x * maxAllowedDistX);
		//	}

		//	if (outsideAllowedDistY)
		//	{
		//		vCamDestPos.y = mCamWorld.trans.y + (vDelta.y - vDeltaNorm.y * maxAllowedDistX);
		//	}

		//	vCamFinalPos = damp? ApproachDamped(mCamWorld.trans, vCamDestPos, deltaTime, 0.99f, timeToTarget, 1.f) : vCamDestPos;
		//}

		////// Approach point that brings the ship into the allowable region
		//////vCamDestPos = mCamWorld.trans + (vDelta - (Normalize(vDelta) * maxAllowedDistXY));



		EulerAngles anchorAngles; anchorAngles.SetFromMatrix(mAnchorWorld);
		EulerAngles shipAngles; shipAngles.SetFromMatrix(mShipWorld);
		EulerAngles camAngles; camAngles.SetFromMatrix(mCamWorld);

		TWEAKABLE float32 camToShipRollRatio = 0.2f;
		TWEAKABLE float32 timeToTarget3 = 2.f;
		EulerAngles camFinalAngles = anchorAngles;
		camFinalAngles.roll = ApproachDamped(camAngles.roll, shipAngles.roll * camToShipRollRatio, deltaTime, 0.99f, timeToTarget3);

		mCamWorld.SetFromEulerAngles(camFinalAngles, vCamFinalPos);

		// Look at the ship
		//const Vector3 vCamCurrForward = mCamWorld.axisZ;
		//const Vector3 vCamTargetForward = Normalize(mShipWorld.trans - vCamFinalPos);
		//TWEAKABLE float32 timeToTarget2 = 2.f;
		//const Vector3 vCamFinalForward = ApproachDirectionDamped(vCamCurrForward, vCamTargetForward, deltaTime, 0.99f, timeToTarget2);//, 0.1f);

		//mCamWorld.SetFromLookAtDir(vCamFinalForward, Vector3::UnitY(), vCamFinalPos);
	}

	virtual void Render()
	{
		//const Matrix43& mCamWorld = GetSceneNode()->GetLocalToWorld();
		//TWEAKABLE float32 scale = 10.f;
		//DebugDrawLine(mCamWorld.trans, mCamWorld.trans + mCamWorld.axisZ * scale);

		//const Matrix43& mShipWorld = m_pwTarget.lock()->GetLocalToWorld();
		//DebugDrawAxes(mShipWorld, 10.f);
	}

	SceneNodeWeakPtr m_pwTarget;
	float32 m_offset;
};


class StaticMeshComponent : public SceneNodeComponent
{
public:
	void Init(const std::shared_ptr<gfx::StaticMesh>& psStaticMesh)
	{
		m_pStaticMesh = std::move(psStaticMesh);
	}

	virtual void Render()
	{
		DrawStaticMesh(*m_pStaticMesh, GetSceneNode()->GetLocalToWorld());
	}

	gfx::StaticMesh& GetMesh()
	{
		return *m_pStaticMesh;
	}

private:
	std::shared_ptr<gfx::StaticMesh> m_pStaticMesh;
};


class MissileControlComponent : public SceneNodeComponent
{
public:
	MissileControlComponent()
		: m_fired(false)
		, m_speed(0.f)
		, m_elapsedTime(0.f)
	{
	}

	bool IsFired() const
	{
		return m_fired;
	}

	void Fire()
	{
		assert(!IsFired());
		m_fired = true;

		// Become a root node
		GetSceneNode()->DetachFromParent();
	}

	virtual void Update(float32 deltaTime)
	{
		if (!m_fired)
			return;

		m_elapsedTime += deltaTime;
		if (m_elapsedTime > 5.f)
		{
			SceneNode::Destroy(GetSceneNode()->AsSharedPtr());
			return;
		}

		static float32 maxMoveSpeed = 1000.f;
		static float32 accel = 100.f;

		m_speed += accel * deltaTime;
		m_speed = MathEx::Max(m_speed, maxMoveSpeed);

		auto& mMissile = GetSceneNode()->ModifyLocalToParent();
		mMissile.trans += m_speed * deltaTime * mMissile.axisZ;
	}

private:
	bool m_fired;
	float32 m_speed;
	float32 m_elapsedTime;
};

// Ship and Ground attaches to Anchor
class AnchorComponent : public SceneNodeComponent
{
public:
	AnchorComponent() : m_totalDistance(0.f) {}

	void Advance(float32 distance)
	{
		m_totalDistance += distance;

		// For now, just move along Z axis
		auto& mLocal = GetSceneNode()->ModifyLocalToParent();
		mLocal.trans += mLocal.axisZ * distance;

		//TWEAKABLE float32 speed = 0.01f;
		//TWEAKABLE float32 amplitude = 100.f;
		//const float32 lastS = MathEx::Sin(speed * (m_totalDistance - distance));
		//const float32 currS = MathEx::Sin(speed * m_totalDistance);
		//const float32 delta = (lastS - currS) * amplitude;
		//mLocal.trans += mLocal.axisX * delta;
	}

	float m_totalDistance;
};


class BufferedKeyboardMgr : public Singleton<BufferedKeyboardMgr>
{
public:
	static const float32 MaxBufferedTime() { return 1.f; }

	float32 ElapsedTimeSinceTimeStamp(float64 timeStamp) { return static_cast<float32>( GetFrameTimer().GetElapsedTime() - timeStamp ); }

	void Update(float32 deltaTime)
	{
		KeyboardMgr& kbMgr = KeyboardMgr::Instance();

		for (auto& kv : m_buffersPerKey)
		{
			// Remove all stale elements except the most recently stale one to make sure
			// we always have a bounding element in the buffer
			std::vector<Data>& buffer = kv.second;
			for (auto iter = buffer.begin(); iter != buffer.end(); )
			{
				Data& data = *iter;
				bool stale = ElapsedTimeSinceTimeStamp(data.m_timeStamp) >= MaxBufferedTime();
				if (stale && (iter+1) != buffer.end()) // Keep newest stale entry (last entry)
				{
					iter = buffer.erase(iter);
				}
				else
				{
					++iter;
				}
			}

			assert(!buffer.empty());

			// Check for key state change, add elem if changed
			const KeyState& currKeyState = kbMgr.GetKeyState(kv.first);
			const KeyState& lastKeyState = buffer.back().m_keyState;

			if (currKeyState != lastKeyState)
			{
				Data data;
				data.m_keyState = currKeyState;
				data.m_timeStamp = GetFrameTimer().GetElapsedTime();
				buffer.insert(buffer.begin(), data); //@TODO: Expensive! change to list?
			}
		}
	}

	const KeyState& GetKeyState(VKEY vkey, float32 timeAgo)
	{
		assert(timeAgo < MaxBufferedTime());

		const auto& buffer = GetBuffer(vkey);
		assert(!buffer.empty());

		// Entries in buffer are ordered from newest to oldest
		const Data* pResult = nullptr;

		for (auto iter = buffer.cbegin(); iter != buffer.cend(); ++iter)
		{
			if (ElapsedTimeSinceTimeStamp(iter->m_timeStamp) >= timeAgo)
			{
				pResult = &*iter;
				break;
			}
		}

		// We're guaranteed to have at least 1 'stale' entry in the buffer that's older than max buffered time
		assert(pResult != nullptr);

		return pResult->m_keyState;
	}

private:
	struct Data
	{
		Data() : m_timeStamp(0.f) {}

		KeyState m_keyState;
		float64 m_timeStamp;
	};	

	const std::vector<Data>& GetBuffer(VKEY vkey) const
	{
		auto result = m_buffersPerKey.insert( std::make_pair(vkey, std::vector<Data>()) );
		if (result.second) // True if just inserted
		{
			Data data;
			data.m_keyState = KeyboardMgr::Instance().GetKeyState(vkey);			
			data.m_timeStamp = GetFrameTimer().GetElapsedTime() - MaxBufferedTime() - 1.0; // Make the first entry stale on purpose
			result.first->second.push_back(data);
		}
		return result.first->second;
	}

	mutable std::map<VKEY, std::vector<Data>> m_buffersPerKey;
};

class PlayerControlComponent : public SceneNodeComponent
{
public:
	PlayerControlComponent()
		: m_speed(1000.f)
		, m_currAngles(EulerAngles::Identity())
		, m_targetAngles(EulerAngles::Identity())
	{
	}

	virtual void Update(float32 deltaTime)
	{
		UpdateOrientation(deltaTime);
		UpdateMovement(deltaTime);
	}

	void UpdateOrientation(float32 deltaTime)
	{
		//KeyboardMgr& kbMgr = KeyboardMgr::Instance();
		BufferedKeyboardMgr& bufferedKbMgr = BufferedKeyboardMgr::Instance();

		TWEAKABLE float32 maxYaw = Angle::FromDeg(30.f);
		TWEAKABLE float32 maxPitch = Angle::FromDeg(30.f);
		TWEAKABLE float32 maxRoll = Angle::FromDeg(30.f);

		TWEAKABLE float32 minInputTime = 0.f;

		TWEAKABLE float32 timeToTarget = 1.25f;
		TWEAKABLE float32 factor = 0.99f;
		TWEAKABLE float32 tolerance = Angle::FromDeg(1.f);

		if (bufferedKbMgr.GetKeyState(vkeyShipLeft, minInputTime).IsDown())
		{
			m_targetAngles.yaw = -maxYaw;
		}
		else if (bufferedKbMgr.GetKeyState(vkeyShipRight, minInputTime).IsDown())
		{
			m_targetAngles.yaw = maxYaw;
		}
		else if (bufferedKbMgr.GetKeyState(vkeyShipLeft, minInputTime).IsUp() && bufferedKbMgr.GetKeyState(vkeyShipRight, minInputTime).IsUp())
		{
			m_targetAngles.yaw = 0.f;
		}

		if (bufferedKbMgr.GetKeyState(vkeyShipDown, minInputTime).IsDown())
		{
			m_targetAngles.pitch = -maxPitch;
		}
		else if (bufferedKbMgr.GetKeyState(vkeyShipUp, minInputTime).IsDown())
		{
			m_targetAngles.pitch = maxPitch;
		}
		else if (bufferedKbMgr.GetKeyState(vkeyShipDown, minInputTime).IsUp() && bufferedKbMgr.GetKeyState(vkeyShipUp, minInputTime).IsUp())
		{
			m_targetAngles.pitch = 0.f;
		}

		m_currAngles.yaw = ApproachDamped(m_currAngles.yaw, m_targetAngles.yaw, deltaTime, factor, timeToTarget, tolerance);
		m_currAngles.pitch = ApproachDamped(m_currAngles.pitch, m_targetAngles.pitch, deltaTime, factor, timeToTarget, tolerance);

		const float32 yawSign = MathEx::Sign<float32>(m_currAngles.yaw);
		m_currAngles.roll = MathEx::MapRangedValue(yawSign * m_currAngles.yaw, 0.f, maxYaw, 0.f, maxRoll) * -yawSign;

		auto& mLocal = GetSceneNode()->ModifyLocalToParent();
		mLocal.SetRotFromEulerAngles(m_currAngles);
	}

	void UpdateMovement(float32 deltaTime)
	{
		auto& mLocal = GetSceneNode()->ModifyLocalToParent();

		Vector3 vMoveDelta = mLocal.axisZ * m_speed * deltaTime;

		// Move ship in local xy plane, but apply local z movement to anchor (parent)
		
		mLocal.trans.x += vMoveDelta.x;
		mLocal.trans.y += vMoveDelta.y;

		auto pAnchorComponent = GetSceneNode()->GetParent()->GetComponent<AnchorComponent>();
		pAnchorComponent->Advance(vMoveDelta.z);
	}

	float32 m_speed;

	EulerAngles m_currAngles;
	EulerAngles m_targetAngles;
};

// Attached to Anchor
class GroundComponent : public SceneNodeComponent
{
public:
	virtual void Render()
	{
		const Matrix43& mWorld = GetSceneNode()->GetLocalToWorld();

		// Draw the ground plane
		TWEAKABLE float32 halfPlaneSizeX = 10000.f;
		TWEAKABLE float32 planeStartZ = 1000.f;
		TWEAKABLE float32 planeEndZ = 9000.f;

		const Vector3& v1 = PositionVector(Vector3(-halfPlaneSizeX, 0.f, -planeStartZ)) * mWorld;
		const Vector3& v2 = PositionVector(Vector3(halfPlaneSizeX, 0.f, -planeStartZ)) * mWorld;
		const Vector3& v3 = PositionVector(Vector3(halfPlaneSizeX, 0.f, planeEndZ)) * mWorld;
		const Vector3& v4 = PositionVector(Vector3(-halfPlaneSizeX, 0.f, planeEndZ)) * mWorld;

		glPushAttrib(GL_LIGHTING_BIT|GL_TEXTURE_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		
		glBegin(GL_QUADS);
		{
			glColor3ub(25, 89, 58);
			glVertex3fv(v1.v);
			glVertex3fv(v2.v);

			glColor3ub(132, 178, 181);
			glVertex3fv(v3.v);
			glVertex3fv(v4.v);
		}
		glEnd();

		// Draw visible white dots
		TWEAKABLE float32 halfDistX = 80.f;
		TWEAKABLE float32 distZ = 200.f;
		TWEAKABLE float32 numDots = 20.f;

		const Vector3 vOffsetX(halfDistX, 0.f, 0.f);

		Vector3 vDotCenter = mWorld.trans;
		vDotCenter.x = 0.f;
		vDotCenter.y += 0.1f;
		vDotCenter.z = MathEx::Floor(vDotCenter.z / distZ) * distZ;
		
		glPointSize(5.0f);
		glBegin(GL_POINTS);
		glColor4f(1.f, 1.f, 1.f, 1.f);
		for (uint32 i = 0; i < numDots; ++i)
		{
			glVertex3fv((vDotCenter - vOffsetX).v);
			glVertex3fv((vDotCenter - vOffsetX * 3.f).v);
			glVertex3fv((vDotCenter - vOffsetX * 5.f).v);
			glVertex3fv((vDotCenter - vOffsetX * 7.f).v);

			glVertex3fv((vDotCenter + vOffsetX).v);
			glVertex3fv((vDotCenter + vOffsetX * 3.f).v);
			glVertex3fv((vDotCenter + vOffsetX * 5.f).v);
			glVertex3fv((vDotCenter + vOffsetX * 7.f).v);

			vDotCenter.z += distZ;
		}
		glEnd();

		glPopAttrib();
		//glPopMatrix();
	}
};


class StarfieldComponent : public SceneNodeComponent
{
public:
	virtual void Render()
	{
		srand(42);

		static uint32 numStars = 10000;
		static float32 halfRange = 1000.f;

		glPushAttrib(GL_LIGHTING_BIT|GL_TEXTURE_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glPointSize(1.0f);
		glBegin(GL_POINTS);
		glColor4f(1.f, 1.f, 1.f, 1.f);
		for (uint32 i = 0; i < 10000; ++i)
		{
			glVertex3f(MathEx::Rand(-halfRange, halfRange), MathEx::Rand(-halfRange, halfRange), MathEx::Rand(-halfRange, halfRange));
		}
		glEnd();
		glPopAttrib();
	}

private:
	uint32 m_randomSeed;
};

int main()
{
	extern void UnitTest_Math();
	UnitTest_Math();

	ScreenMode::Type screenMode = ScreenMode::Windowed;
	VertSync::Type vertSync = VertSync::Disable;

	GraphicsEngine& gfxEngine = GraphicsEngine::Instance();
	gfxEngine.Initialize("Star Fox", (int)SCREEN_WIDTH, (int)SCREEN_HEIGHT, 32, screenMode, vertSync);

	//Viewport vp(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	//gfxEngine.SetActiveViewport(vp);

	//System::SetMouseVisible(true);

	// By default, when lighting is enabled, material parameters are used for shading, but vertex colors are ignored;
	// while the opposite is true when lighting is disabled.
	// Enabling GL_COLOR_MATERIAL tells OpenGL to substitute the current color for the ambient and for the diffuse material
	// color when doing lighting computations.
	glEnable(GL_COLOR_MATERIAL);

	GLUtil::SetBlending(false);
	GLUtil::SetDepthTesting(true);
	GLUtil::SetFrontFace(Winding::CounterClockwise, CullBackFace::False);
	GLUtil::SetLighting(true);
	GLUtil::SetShadeModel(ShadeModel::Smooth);
	GLUtil::SetTexturing(true);

	ProjectionInfo perspMain;
	perspMain.SetPerspective(45.f, SCREEN_WIDTH / SCREEN_HEIGHT, 1.0f, 10000.f);
	GLUtil::SetProjection(perspMain);

	//GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	//GLfloat mat_shininess[] = { 50.0 };
	//glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_specular);
	//glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	//glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	GLfloat light_position[] = { 0.0, 0.0, 1.0, 0.0 }; // Directional in -z direction
	//GLfloat light_position[] = { 0.0, 0.0, 0.0, 1.0 }; // Point at camera loc
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHT0);


	FrameTimer& frameTimer = GetFrameTimer();
	frameTimer.SetMinFPS(10.0f);
	//frameTimer.SetMaxFPS(60.f);

	KeyboardMgr& kbMgr = KeyboardMgr::Instance();
	BufferedKeyboardMgr& bufferedKbMgr = BufferedKeyboardMgr::Instance();

	// Create SceneNodes

	SceneNodeWeakPtr pwCamera;

	{
		FbxLoader fbxLoader;
		fbxLoader.Init();

		auto psShip = SceneNode::Create("Ship");
		auto psStaticMesh = fbxLoader.LoadStaticMesh("data/Arwing_001.fbx");
		psShip->AddComponent<StaticMeshComponent>()->Init(psStaticMesh);
		psShip->AddComponent<PlayerControlComponent>();
		psShip->ModifyLocalToWorld().trans.y = 50.f; // The ship starts above the ground

		const Vector3 shipExtents = psStaticMesh->m_boundingBox.GetExtents();

		//psShip->AddComponent<PlayerControlComponent>();
		//std::vector<PlayerControlComponent*> r = psShip->TryGetComponents<PlayerControlComponent>();
		//assert(r.size() == 2);

		//psStaticMesh = fbxLoader.LoadStaticMesh("data/Granga_Missile.fbx");
		//for (uint32 i = 0; i < 4; ++i)
		//{
		//	auto psMissile = SceneNode::Create(str_format("Missile_%d", i));
		//	psMissile->AddComponent<StaticMeshComponent>()->Init(psStaticMesh);
		//	psMissile->AddComponent<MissileControlComponent>();

		//	psShip->AttachChild(psMissile);
		//	Matrix43 mScale;
		//	mScale.SetFromScaleVector(Vector3(0.2f, 0.2f, 0.2f));
		//	psMissile->ModifyLocalToParent() = mScale * psShip->GetComponent<StaticMeshComponent>()->GetMesh().m_sockets[i].m_matrix;
		//}
		psStaticMesh = nullptr;

		auto psGround = SceneNode::Create("Ground");
		psGround->AddComponent<GroundComponent>();		
		
		auto psAnchor = SceneNode::Create("Anchor");
		psAnchor->AddComponent<AnchorComponent>();
		psAnchor->AttachChild(psShip);
		psAnchor->AttachChild(psGround);

		//const auto& allMissileComponents = psShip->GetComponentsInChildren<MissileControlComponent>();
		//assert(allMissileComponents.size() == 4);

		auto psCamera = SceneNode::Create("Camera");
		//auto pOrbitTarget = psCamera->AddComponent<OrbitTargetCameraComponent>();
		auto pCamerComponent = psCamera->AddComponent<FollowShipCameraComponent>();		
		pCamerComponent->SetTarget(psShip);
		pwCamera = psCamera;

		//auto pStarfield = SceneNode::Create("Starfield");
		//pStarfield->AddComponent<StarfieldComponent>();

		// Create a bunch of randomly positioned buildings
		{
			const float32 firstZ = 5000.f;
			const float32 deltaZ = 2000.f;

			auto psStaticMesh = fbxLoader.LoadStaticMesh("data/Building1.fbx");

			for (uint32 i = 0; i < 100; ++i)
			{
				auto psBuilding = SceneNode::Create(str_format("Building_%d", i));
				psBuilding->AddComponent<StaticMeshComponent>()->Init(psStaticMesh);

				auto& mLocal = psBuilding->ModifyLocalToParent();
				mLocal.trans.z = firstZ + deltaZ * i;
				mLocal.trans.x = MathEx::Rand(-500.f, 500.f);
			}
			psStaticMesh = nullptr;
		}

		fbxLoader.Shutdown();
	}

	// Main game loop
	bool bQuit = false;
	while (!bQuit)
	{
		// Handle pause
		if ( !gfxEngine.HasFocus() ) // Auto-pause when we window loses focus
		{
			frameTimer.SetPaused(true);
		}
		else if (kbMgr[vkeyPause].JustPressed())
		{
			frameTimer.TogglePaused();
		}

		// Frame time update
		frameTimer.Update();

		// Update and apply time scale
		//@TODO: Fold this into frameTimer?
		static float32 timeScales[] = {0.1f, 0.25f, 0.5f, 1.f, 2.f, 4.f};
		static int32 timeScaleIndex = 3;
		if (kbMgr[vkeyTimeScaleInc].JustPressed())
		{
			timeScaleIndex = MathEx::Min(timeScaleIndex+1, (int)ARRAYSIZE(timeScales)-1);
		}
		else if (kbMgr[vkeyTimeScaleDec].JustPressed())
		{
			timeScaleIndex = MathEx::Max(timeScaleIndex-1, 0);
		}
		else if (kbMgr[vkeyTimeScaleReset].JustPressed())
		{
			timeScaleIndex = 3;
		}
		const float32 timeScale = timeScales[timeScaleIndex];

		const float32 deltaTime = timeScale * frameTimer.GetFrameDeltaTime();

		gfxEngine.SetTitle( 
			str_format("Star Fox (Real Time: %.2f, Game Time: %.2f, GameDT: %.4f (scale: %.2f), FPS: %.2f)",
			frameTimer.GetRealElapsedTime(),
			frameTimer.GetElapsedTime(),
			frameTimer.GetFrameDeltaTime(),
			timeScale,
			frameTimer.GetFPS()).c_str() );

		kbMgr.Update(deltaTime);
		bufferedKbMgr.Update(deltaTime);

		if ( frameTimer.IsPaused() )
		{
			System::Sleep(1);
		}
		else
		{
			if (kbMgr[VK_CONTROL].IsDown())
			{
				if (kbMgr[VK_F1].JustPressed())
				{
					static bool bLighting = glIsEnabled(GL_LIGHTING) == GL_TRUE;
					bLighting = !bLighting;
					bLighting? glEnable(GL_LIGHTING) : glDisable(GL_LIGHTING);
				}
				if (kbMgr[VK_F2].JustPressed())
				{
					//static bool bSmoothShading = GLUtil::GetShadeModel() == ShadeModel::Smooth;
					//bSmoothShading = !bSmoothShading;
					//GLUtil::SetShadeModel(bSmoothShading? ShadeModel::Smooth : ShadeModel::Flat);
					g_drawSockets = !g_drawSockets;
				}
				if (kbMgr[VK_F3].JustPressed())
				{
					g_drawNormals = !g_drawNormals;
				}
				if (kbMgr[VK_F4].JustPressed())
				{
					GLUtil::SetTexturing( !GLUtil::GetTexturing() );
				}
				if (kbMgr[VK_F5].JustPressed())
				{
					static bool bWireframe = false;
					bWireframe = !bWireframe;
					GLUtil::SetWireFrame(bWireframe);
				}
				if (kbMgr[VK_F6].JustPressed())
				{
					g_renderSceneGraph = !g_renderSceneGraph;
				}
			}

			// UPDATE			
			std::vector<SceneNodeWeakPtr> sceneNodeList = SceneNode::GetAllNodesSnapshot();
			for (auto pwNode : sceneNodeList)
			{
				if (const auto& psNode = pwNode.lock())
					psNode->Update(deltaTime);
			}

			SceneNode::ValidateSceneGraph();


			// RENDER
			glClearColor(0.f, 0.f, 0.3f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Load inverse camera matrix so future transforms are in camera space
			glMatrixMode(GL_MODELVIEW);
			//glLoadIdentity();
			Matrix43 mInvCam = pwCamera.lock()->GetLocalToWorld();
			//assert(mInvCam.IsOrthogonal());
			mInvCam.axisZ = -mInvCam.axisZ; // Game -> OpenGL (flip Z axis)
			mInvCam.InvertSRT();
			GLfloat mCamGL[16];
			GLUtil::Matrix43ToGLMatrix(mInvCam, mCamGL);
			glLoadMatrixf(mCamGL);

			// Render scene nodes in camera space
			for (auto pwNode : sceneNodeList)
			{
				if (const auto& psNode = pwNode.lock())
				{
					psNode->Render();
				}
			}

			// Render debug objects
			g_debugDrawManager.Render();

			// Render scene graph
			if (g_renderSceneGraph)
			{
				for (auto pwNode : sceneNodeList)
				{
					if (const auto& psNode = pwNode.lock())
					{
						if (is_weak_to_shared_ptr(pwCamera, psNode))
							continue;

						GLUtil::PushAndMultMatrix(psNode->GetLocalToWorld());

						auto pQuadric = gluNewQuadric();
						glColor3f(1.f, 0.f, 0.f);
						gluSphere(pQuadric, 10.f, 10, 10);
						gluDeleteQuadric(pQuadric);

						for (const auto& pwChildNode : psNode->GetChildren())
						{
							if (const auto& psChildNode = pwChildNode.lock())
							{
								const auto& mChildLocal = psChildNode->GetLocalToParent();

								glBegin(GL_LINES);
								glVertex3fv(Vector3::Zero().v);
								glVertex3fv(mChildLocal.trans.v);
								glEnd();
							}
						}
						
						glPopMatrix();
					}
				}
			}
		}

		// Flip buffers, process msgs, etc.
		gfxEngine.Update(bQuit, frameTimer.IsPaused());

		// Handle frame stepping
		static bool stepFrame = false;
		const bool inputStepSingleFrame = kbMgr[vkeyStepFrame].JustPressed();
		const bool inputStepMultipleFrames = kbMgr[vkeyStepFrameRepeatModifier].IsDown() && kbMgr[vkeyStepFrame].IsDown();
		if ( !stepFrame && (inputStepSingleFrame || inputStepMultipleFrames) )
		{
			// Unpause for one frame
			stepFrame = true;
			frameTimer.SetPaused(false);
		}
		else if ( stepFrame && !inputStepMultipleFrames )
		{
			// Go back to being paused
			stepFrame = false;
			frameTimer.SetPaused(true);
		}
	}

	SceneNode::DestroyAllNodes();

	gfxEngine.Shutdown();
}
