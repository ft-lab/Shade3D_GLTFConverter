/**
 * アニメーション情報.
 */
#include "AnimationData.h"

//-----------------------------------------------------------------------.
CAnimChannelData::CAnimChannelData ()
{
	clear();
}

void CAnimChannelData::clear ()
{
	samplerIndex    = -1;
	targetNodeIndex = -1;
	pathType        = CAnimChannelData::path_type_translation;
}

//-----------------------------------------------------------------------.
CAnimSamplerData::CAnimSamplerData ()
{
	clear();
}

void CAnimSamplerData::clear ()
{
	inputData.clear();
	interpolationType = CAnimSamplerData::interpolation_type_linear;
	outputData.clear();
}

//-----------------------------------------------------------------------.
CAnimationData::CAnimationData ()
{
	clear();
}

void CAnimationData::clear ()
{
	name = "";
	channelData.clear();
	samplerData.clear();
}

/**
 * アニメーション情報を持つかどうか.
 */
bool CAnimationData::hasAnimation () const
{
	return (!channelData.empty() && !samplerData.empty());
}
