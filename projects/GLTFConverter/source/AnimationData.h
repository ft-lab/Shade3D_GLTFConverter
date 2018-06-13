/**
 * アニメーション情報.
 */
#ifndef _ANIMATIONDATA_H
#define _ANIMATIONDATA_H

#include "GlobalHeader.h"

#include <vector>
#include <string>

//-----------------------------------------------------------------------.
// アニメーションで使用するノードと参照するSamplerの関連付け用.
//-----------------------------------------------------------------------.
class CAnimChannelData
{
public:
	// 要素の種類.
	enum PATH_TYPE {
		path_type_none = 0,
		path_type_translation,			// 移動.
		path_type_rotation,				// 回転.
		path_type_scale,				// スケール.
		path_type_weights,				// Weights.
	};

public:
	int samplerIndex;			// Samplerの番号.
	int targetNodeIndex;		// ターゲットのノード番号.
	PATH_TYPE pathType;			// 0:translation / 1:rotation / 2:scale のいずれか.


public:
	CAnimChannelData ();

	CAnimChannelData& operator = (const CAnimChannelData &v) {
		this->samplerIndex    = v.samplerIndex;
		this->targetNodeIndex = v.targetNodeIndex;
		this->pathType        = v.pathType;
		return (*this);
	}

	void clear ();
};

//-----------------------------------------------------------------------.
// アニメーション時の1つの要素の情報.
//-----------------------------------------------------------------------.
class CAnimSamplerData
{
public:
	// キーフレーム間の補間の種類.
	enum INTERPOLATION_TYPE {
		interpolation_type_linear = 0,
		interpolation_type_smooth,
	};

public:
	std::vector<float> inputData;				// 秒単位のキーフレーム位置.
	INTERPOLATION_TYPE interpolationType;		// キーフレーム間の補間の種類.
	std::vector<float> outputData;				// キーフレームごとの、translation(vec3)/rotation(quaternion)/scale(vec3)の配列.

public:
	CAnimSamplerData ();

	CAnimSamplerData& operator = (const CAnimSamplerData &v) {
		this->inputData         = v.inputData;
		this->interpolationType = v.interpolationType;
		this->outputData        = v.outputData;
		return (*this);
	}

	void clear ();
};

//-----------------------------------------------------------------------.
// アニメーションデータ管理クラス.
//-----------------------------------------------------------------------.
class CAnimationData
{
public:
	std::string name;		// アニメーション名.

	std::vector<CAnimChannelData> channelData;
	std::vector<CAnimSamplerData> samplerData;

public:
	CAnimationData ();

    CAnimationData& operator = (const CAnimationData &v) {
		this->name         = v.name;
		this->channelData  = v.channelData;
		this->samplerData  = v.samplerData;
		return (*this);
	}

	void clear ();

	/**
	 * アニメーション情報を持つかどうか.
	 */
	bool hasAnimation () const;
};

#endif
