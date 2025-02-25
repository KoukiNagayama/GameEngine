#include "stdafx.h"
#include "Bell.h"
#include "sound/SoundEngine.h"
#include "Player.h"
#include "GameCamera.h"
#include "InfoForEdge.h"

namespace
{
	const float POSITION = 60.0f;
	const float TIMER = 3.0f;
	const float VOLUME = 1.0f;
	const float SOUND_RANGE = 800.0f;
	const float EDGE_FADE_IN_DELTA_VALUE = 0.07f;	// エッジがフェードインするときの変位量
	const float EDGE_FADE_OUT_DELTA_VALUE = 0.01f;	// エッジがフェードアウトするときの変位量
	const float RATE_BY_TIME_MAX_VALUE = 1.00f;		// 時間による影響率の最大値
	const float RATE_BY_TIME_MIN_VALUE = 0.00f;		// 時間による影響率の最小値
	const float MODEL_MULTIPLIER = 35.0f;			//カメラの前方向、右方向に乗算する値。
	const float MODEL_UP = 25.0f;					//カメラの上方向に乗算する値
	const float SOUND_MULTIPLIER = 15.0f;			//ベルの前方向に乗算する値
	const float LIFE_TIME = 2.0f;					//強制的に音源を死亡させるライフタイム
}

Bell::Bell()
{
}
Bell::~Bell()
{
	g_infoForEdge.SetIsSound(0, 0);
	g_infoForEdge.SetRate(0, 0.00f);
	if (m_bellSound != nullptr) {
		DeleteGO(m_bellSound);
	}
}

bool Bell::Start()
{
	m_player = FindGO<Player>("player");
	m_gameCam = FindGO<GameCamera>("gamecamera");
	//サウンドを登録。
	g_soundEngine->ResistWaveFileBank(0, "Assets/sound/item/bell_low.wav");
	//モデルの初期化。
	m_modelRender.Init("Assets/modelData/item/bell.tkm");

	g_infoForEdge.InitForSound(0, m_position, SOUND_RANGE, 0, m_rateByTime);

	return true;
}

void Bell::Update()
{
	//座標
	Position();
	//ステート管理
	ManageState();
	// デバッグ用の文字表示
	Font();
	//影響率
	CheckRate();
	//モデルの更新。
	m_modelRender.Update();
}

void Bell::Position()
{
	Vector3 up, right, forward;
	right = g_camera3D->GetRight();
	forward = g_camera3D->GetForward();
	up.Cross(right, forward);
	up.Normalize();
	//ベルのモデルの座標を計算する。
	m_position = g_camera3D->GetPosition();
	m_position += g_camera3D->GetRight() * MODEL_MULTIPLIER;
	m_position += g_camera3D->GetForward() * MODEL_MULTIPLIER;
	m_position += up * MODEL_UP;
	//座標を設定する
	m_modelRender.SetPosition(m_position);
	m_modelRender.SetRotation(m_player->GetRotation());

	//ベルの音の座標を少し前に設定する。
	m_soundPos = g_camera3D->GetPosition();
	m_soundPos += g_camera3D->GetForward() * SOUND_MULTIPLIER;

}

void Bell::TransitionState()
{
	if (g_pad[0]->IsTrigger(enButtonB))
	{
		//音を鳴らす
		MakeSound();
		if (m_isNeverRung == true) {
			m_isNeverRung = false;
		}
		//Ringステートにする
		m_bellState = enBellState_Ring;
		//クールタイムのセット
		m_timer = TIMER;
		m_isRing = true;
		return;
	}
	m_bellState = enBellState_Idle;
	m_isRing = false;
}

void Bell::ManageState()
{
	switch (m_bellState)
	{
		//待機。
	case enBellState_Idle:
		Idle();
		break;
		//音を鳴らす。
	case enBellState_Ring:
		Ring();
		break;
	default:
		break;
	}
}

void Bell::Ring()
{
	if (m_timer <= 0.0f)
	{
		m_timer = 0.0f;
		m_bellState = enBellState_Idle;
		return;
	}
	m_timer -= g_gameTime->GetFrameDeltaTime();
}

void Bell::Idle()
{
	TransitionState();
}

void Bell::MakeSound()
{
	m_bellSound = NewGO<SoundSource>(0);
	m_bellSound->Init(0);
	m_bellSound->SetVolume(VOLUME);
	m_bellSound->Play(false, LIFE_TIME);
}

void Bell::CheckRate()
{
	int check1;
	if (m_bellSound != nullptr) {
		if (m_bellSound->IsPlaying() == true)
		{
			check1 = 1;
			if (m_rateByTime < RATE_BY_TIME_MAX_VALUE) {
				m_rateByTime += EDGE_FADE_IN_DELTA_VALUE;
			}
		}
		else {
			check1 = 0;
			if (m_rateByTime > RATE_BY_TIME_MIN_VALUE && check1 == 0) {
				m_rateByTime -= EDGE_FADE_OUT_DELTA_VALUE;
				if (m_rateByTime <= RATE_BY_TIME_MIN_VALUE) {
					m_rateByTime = RATE_BY_TIME_MIN_VALUE;
					m_bellSound = nullptr;
				}
			}
		}
		g_infoForEdge.SetInfoForSound(0, m_soundPos, SOUND_RANGE, check1, m_rateByTime);
	}
}

void Bell::Render(RenderContext& rc)
{
	m_modelRender.Draw(rc);
}