/**
 * @file   ShapeStack.h
 * @brief  エクスポート時のシーン構築用のスタック.
 */

#ifndef _SHAPESTACK_H
#define _SHAPESTACK_H

#include "GlobalHeader.h"

#include <vector>

typedef struct {
	int depth;						///< 階層の深さ.
	sxsdk::shape_class *pShape;		///< 形状へのポインタ.
	sxsdk::mat4 tMat;				///< 形状の変換行列.
} SHAPE_STACK_NODE;

/**
 * 形状および変換行列のスタック管理クラス.
 */
class CShapeStack
{
private:
	std::vector<SHAPE_STACK_NODE> m_stackNode;

	sxsdk::mat4 m_LWMat;	///< ルートからの累積変換行列.

public:
	CShapeStack ();
	~CShapeStack ();

	/**
	 * 情報のクリア.
	 */
	void clear ();

	/**
	 * 情報のプッシュ.
	 * @param[in] depth        階層の深さ.
	 * @param[in] pShape       形状情報のポインタ.
	 * @param[in] tMat         変換行列.
	 */
	void push (const int depth, sxsdk::shape_class *pShape, sxsdk::mat4 tMat);

	/**
	 * 情報のポップ.
	 */
	void pop ();

	/**
	 * 現在のカレント情報を取得.
	 */
	int getShape (sxsdk::shape_class **ppRetShape, sxsdk::mat4 *pRetMat, int *pRetDepth = NULL);

	/**
	 * 現在の変換行列を取得.
	 */
	sxsdk::mat4 getLocalToWorldMatrix () { return m_LWMat; }
	sxsdk::mat4 getWorldToLocalMatrix () { return inv(m_LWMat); }

	/**
	 * スタックの要素数の取得.
	 */
	inline int getElementCount () { return (int)m_stackNode.size(); }

	/**
	 * 要素へのポインタを取得.
	 */
	inline SHAPE_STACK_NODE *getShapeStackPointer () { return &m_stackNode[0]; }

	/**
	 * 最後の要素へのポインタを取得.
	 */
	inline SHAPE_STACK_NODE *getLast () { return (&m_stackNode[0]) + ((int)m_stackNode.size() - 1); }

	/**
	 * LW行列を計算.
	 */
	sxsdk::mat4 calcLocalToWorldMatrix (const bool use_last);

	/**
	 * カレントからルートまでの形状を配列で取得（リンク処理用）.
	 */
	int getShapes (std::vector<sxsdk::shape_class *> &shapes);

	/**
	 * 面反転を取得.
	 */
	bool isFlipFace ();

	/**
	 * 表面材質情報を持つ親までたどる.
	 */
	sxsdk::shape_class* getHasSurfaceShape ();
};

#endif
