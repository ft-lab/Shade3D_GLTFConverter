/**
 * @file   ShapeStack.cpp
 * @brief  エクスポート時のシーン構築用のスタック.
 */

#include "ShapeStack.h"

#include <stdio.h>
#include <stdlib.h>

CShapeStack::CShapeStack ()
{
	m_LWMat = sxsdk::mat4::identity;
	m_stackNode.clear();
}

CShapeStack::~CShapeStack ()
{
	m_stackNode.clear();
}

/**
 * 情報のクリア.
 */
void CShapeStack::clear ()
{
	m_stackNode.clear();
	m_LWMat = sxsdk::mat4::identity;
}

/**
 * 情報のプッシュ.
 * @param[in] depth        階層の深さ.
 * @param[in] pShape       形状情報のポインタ.
 * @param[in] tMat         変換行列.
 */
void CShapeStack::push (const int depth, sxsdk::shape_class *pShape, sxsdk::mat4 tMat)
{
	SHAPE_STACK_NODE node;

	node.depth       = depth;
	node.pShape      = pShape;
	node.tMat        = tMat;
	m_stackNode.push_back(node);

	m_LWMat = tMat * m_LWMat;
}

/**
 * 情報のポップ.
 */
void CShapeStack::pop ()
{
	SHAPE_STACK_NODE *pNode;

	int size = (int)m_stackNode.size();
	if (size <= 0) return;
	pNode = &(m_stackNode[size - 1]);
	const sxsdk::mat4 tMat = pNode->tMat;
	m_stackNode.pop_back();
	size--;

	if (size >= 1) {
		m_LWMat = inv(tMat) * m_LWMat;
	} else {
		m_LWMat = sxsdk::mat4::identity;
	}
}

/**
 * 現在のカレント情報を取得.
 */
int CShapeStack::getShape (sxsdk::shape_class **ppRetShape, sxsdk::mat4 *pRetMat, int *pRetDepth)
{
	SHAPE_STACK_NODE *pNode;

	if (ppRetShape) *ppRetShape = NULL;
	if (pRetMat)    *pRetMat    = sxsdk::mat4::identity;
	if (pRetDepth)  *pRetDepth  = 0;

	if (!m_stackNode.size()) return 0;

	pNode = (&m_stackNode[0]) + (m_stackNode.size()) - 1;

	if (ppRetShape) *ppRetShape = pNode->pShape;
	if (pRetMat)    *pRetMat    = pNode->tMat;
	if (pRetDepth)  *pRetDepth  = pNode->depth;

	return 1;
}

sxsdk::mat4 CShapeStack::calcLocalToWorldMatrix (const bool use_last)
{
	sxsdk::mat4 m = sxsdk::mat4::identity;
	int size = m_stackNode.size();
	if (!use_last) size--;
	for (int i = 0; i < size; ++i) {
		m *= m_stackNode[i].tMat;
	}
	return m;
}

int CShapeStack::getShapes (std::vector<sxsdk::shape_class *> &shapes)
{
	const int size = m_stackNode.size();
	shapes.resize(size);
	for (int i = 0; i < size; ++i) shapes[i] = m_stackNode[size - i - 1].pShape;
	return size;
}

/**
 * 面反転を取得.
 */
bool CShapeStack::isFlipFace ()
{
	const int size = m_stackNode.size();
	bool flipF = m_stackNode[size - 1].pShape->get_flip_face();
	for (int i = size - 2; i >= 0; --i) {
		flipF = (m_stackNode[i].pShape->get_flip_face()) ? !flipF : flipF;
	}
	return flipF;
}

/**
 * 表面材質情報を持つ親までたどる.
 */
sxsdk::shape_class* CShapeStack::getHasSurfaceShape ()
{
	sxsdk::shape_class* pSurfaceShape = NULL;
	const int size = m_stackNode.size();
	for (int i = size - 1; i >= 0; --i) {
		if (m_stackNode[i].pShape->get_has_surface_attributes()) {
			pSurfaceShape = m_stackNode[i].pShape;
			break;
		}
	}
	return pSurfaceShape;
}

