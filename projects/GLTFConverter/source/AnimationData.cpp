/**
 * アニメーション情報.
 */
#include "AnimationData.h"

//-----------------------------------------------------------------------.
CAnimChannelData::CAnimChannelData ()
{
	clear();
}

CAnimChannelData::CAnimChannelData (const CAnimChannelData& v)
{
	this->samplerIndex    = v.samplerIndex;
	this->targetNodeIndex = v.targetNodeIndex;
	this->pathType        = v.pathType;
}
CAnimChannelData::~CAnimChannelData ()
{
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

CAnimSamplerData::CAnimSamplerData (const CAnimSamplerData &v)
{
	this->inputData         = v.inputData;
	this->interpolationType = v.interpolationType;
	this->outputData        = v.outputData;
}
CAnimSamplerData::~CAnimSamplerData ()
{
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
CAnimationData::CAnimationData (const CAnimationData& v)
{
	this->name         = v.name;
	this->channelData  = v.channelData;
	this->samplerData  = v.samplerData;
}
CAnimationData::~CAnimationData ()
{
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
