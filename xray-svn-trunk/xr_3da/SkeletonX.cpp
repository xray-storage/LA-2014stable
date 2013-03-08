// SkeletonX.cpp: implementation of the CSkeletonX class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable:4995)
#include <d3dx9.h>
#pragma warning(default:4995)

#ifndef _EDITOR
    #include	"Render.h"
#endif
#include "SkeletonX.h"
#include "SkeletonCustom.h"
#include "fmesh.h"
#include "xrCPU_Pipe.h"

shared_str	s_bones_array_const;

//////////////////////////////////////////////////////////////////////
// Body Part
//////////////////////////////////////////////////////////////////////
void CSkeletonX::AfterLoad	(CKinematics* parent, u16 child_idx)
{
	SetParent				(parent);
    ChildIDX				= child_idx;
}
void CSkeletonX::_Copy		(CSkeletonX *B)
{
	Parent					= NULL;
	ChildIDX				= B->ChildIDX;
	Vertices1W				= B->Vertices1W;
	Vertices2W				= B->Vertices2W;
	BonesUsed				= B->BonesUsed;

	// caution - overlapped (union)
	cache_DiscardID			= B->cache_DiscardID;
	cache_vCount			= B->cache_vCount;
	cache_vOffset			= B->cache_vOffset;
	RenderMode				= B->RenderMode;
	RMS_boneid				= B->RMS_boneid;
	RMS_bonecount			= B->RMS_bonecount;
}
//////////////////////////////////////////////////////////////////////
void CSkeletonX::_Render	(ref_geom& hGeom, u32 vCount, u32 iOffset, u32 pCount)
{
	RCache.stat.r.s_dynamic.add		(vCount);
	switch (RenderMode)
	{
	case RM_SKINNING_SOFT:
		_Render_soft		(hGeom,vCount,iOffset,pCount);
		RCache.stat.r.s_dynamic_sw.add	(vCount);
		break;
	case RM_SINGLE:	
		{
			Fmatrix	W;	W.mul_43	(RCache.xforms.m_w,Parent->LL_GetTransform_R	(u16(RMS_boneid)));
			RCache.set_xform_world	(W);
			RCache.set_Geometry		(hGeom);
			RCache.Render			(D3DPT_TRIANGLELIST,0,0,vCount,iOffset,pCount);
			RCache.stat.r.s_dynamic_inst.add	(vCount);
		}
		break;
	case RM_SKINNING_1B:
	case RM_SKINNING_2B:
		{
			// transfer matrices
			ref_constant			array	= RCache.get_c				(s_bones_array_const);
			u32						count	= RMS_bonecount;
			for (u32 mid = 0; mid<count; mid++)	{
				Fmatrix&	M				= Parent->LL_GetTransform_R				(u16(mid));
				u32			id				= mid*3;
				RCache.set_ca	(&*array,id+0,M._11,M._21,M._31,M._41);
				RCache.set_ca	(&*array,id+1,M._12,M._22,M._32,M._42);
				RCache.set_ca	(&*array,id+2,M._13,M._23,M._33,M._43);
			}

			// render
			RCache.set_Geometry		(hGeom);
			RCache.Render			(D3DPT_TRIANGLELIST,0,0,vCount,iOffset,pCount);

			if (RM_SKINNING_1B==RenderMode)	RCache.stat.r.s_dynamic_1B.add	(vCount);
			else							RCache.stat.r.s_dynamic_2B.add	(vCount);
		}
		break;
	}
}
void CSkeletonX::_Render_soft	(ref_geom& hGeom, u32 vCount, u32 iOffset, u32 pCount)
{
	u32 vOffset				= cache_vOffset;

	_VertexStream&	_VS		= RCache.Vertex;
	if (cache_DiscardID!=_VS.DiscardID() || vCount>=cache_vCount )
	{
		vertRender*	Dest	= (vertRender*)_VS.Lock(vCount,hGeom->vb_stride,vOffset);
		cache_DiscardID		= _VS.DiscardID();
		cache_vCount		= vCount;
		cache_vOffset		= vOffset;
		
		Device.Statistic->RenderDUMP_SKIN.Begin	();
		if (*Vertices1W)
		{
			PSGP.skin1W(
				Dest,										// dest
				*Vertices1W,								// source
				vCount,										// count
				Parent->bone_instances						// bones
				);
		} else {
			PSGP.skin2W(
				Dest,										// dest
				*Vertices2W,								// source
				vCount,										// count
				Parent->bone_instances						// bones
				);
		}
		Device.Statistic->RenderDUMP_SKIN.End	();
		_VS.Unlock			(vCount,hGeom->vb_stride);
	}

	RCache.set_Geometry		(hGeom);
	RCache.Render			(D3DPT_TRIANGLELIST,vOffset,0,vCount,iOffset,pCount);
}

//////////////////////////////////////////////////////////////////////
extern int ps_r1_SoftwareSkinning;
void CSkeletonX::_Load	(const char* N, IReader *data, u32& dwVertCount) 
{	
	s_bones_array_const	= "sbones_array";
#pragma todo("container is created in stack!")
	xr_vector<u16>	bids;

	// Load vertices
	R_ASSERT	(data->find_chunk(OGF_VERTICES));
			
	//u16			hw_bones_cnt		= u16((HW.Caps.geometry.dwRegisters-22)/3);
	//	Igor: some shaders in r1 need more free constant registers
	u16			hw_bones_cnt		= u16((HW.Caps.geometry.dwRegisters-22-3)/3);

	if ( ps_r1_SoftwareSkinning == 1 )
		hw_bones_cnt = 0;

	u16			sw_bones_cnt		= 0;
#ifdef _EDITOR
	hw_bones_cnt					= 0;
#endif

	u32								dwVertType,size,it,crc;
	dwVertType						= data->r_u32(); 
	dwVertCount						= data->r_u32();

	RenderMode						= RM_SKINNING_SOFT;
	Render->shader_option_skinning	(-1);
	
	switch(dwVertType)
	{
	case OGF_VERTEXFORMAT_FVF_1L: // 1-Link
	case 1:
		{
			size					= dwVertCount*sizeof(vertBoned1W);
			vertBoned1W* pVO		= (vertBoned1W*)data->pointer();

			for (it=0; it<dwVertCount; ++it)
			{
				const vertBoned1W& VB = pVO[it];
				u16 mid				= (u16)VB.matrix;
				
				if(bids.end() == std::find(bids.begin(),bids.end(),mid))	
					bids.push_back	(mid);

				sw_bones_cnt		= _max(sw_bones_cnt, mid);
			}
#ifdef _EDITOR
			// software
			crc						= crc32	(data->pointer(),size);
			Vertices1W.create		(crc,dwVertCount,(vertBoned1W*)data->pointer());
#else
			if(1==bids.size())	
			{
				// HW- single bone
				RenderMode						= RM_SINGLE;
				RMS_boneid						= *bids.begin();
				Render->shader_option_skinning	(0);
			}else 
			if(sw_bones_cnt<=hw_bones_cnt) 
			{
				// HW- one weight
				RenderMode						= RM_SKINNING_1B;
				RMS_bonecount					= sw_bones_cnt+1;
				Render->shader_option_skinning	(1);
			}else 
			{
				// software
				crc								= crc32	(data->pointer(),size);
				Vertices1W.create				(crc,dwVertCount,(vertBoned1W*)data->pointer());
				Render->shader_option_skinning	(-1);
			}
#endif        
		}
		break;
	case OGF_VERTEXFORMAT_FVF_2L: // 2-Link
	case 2:
		{
			size								= dwVertCount*sizeof(vertBoned2W);
			vertBoned2W* pVO					= (vertBoned2W*)data->pointer();

			for(it=0; it<dwVertCount; ++it)
			{
				const vertBoned2W& VB			= pVO[it];
				sw_bones_cnt					= _max(sw_bones_cnt, VB.matrix0);
				sw_bones_cnt					= _max(sw_bones_cnt, VB.matrix1);

				if(bids.end() == std::find(bids.begin(),bids.end(),VB.matrix0))	
					bids.push_back(VB.matrix0);

				if(bids.end() == std::find(bids.begin(),bids.end(),VB.matrix1))	
					bids.push_back(VB.matrix1);
			}
//.			R_ASSERT(sw_bones_cnt<=hw_bones_cnt);
			if(sw_bones_cnt<=hw_bones_cnt)
			{
				// HW- two weights
				RenderMode						= RM_SKINNING_2B;
				RMS_bonecount					= sw_bones_cnt+1;
				Render->shader_option_skinning	(2);
			}
			else 
			{
				// software
				crc								= crc32	(data->pointer(),size);
				Vertices2W.create				(crc,dwVertCount,(vertBoned2W*)data->pointer());
				Render->shader_option_skinning	(-1);
			}
		}
		break;
	default:
		Debug.fatal	(DEBUG_INFO,"Invalid vertex type in skinned model '%s'",N);
		break;
	}
#ifdef _EDITOR
	if (bids.size()>0)	
#else
	if (bids.size()>1)	
#endif
    {
		crc					= crc32(&*bids.begin(),bids.size()*sizeof(u16)); 
		BonesUsed.create	(crc,bids.size(),&*bids.begin());
	}
}

BOOL	CSkeletonX::has_visible_bones		()
{
	if	(RM_SINGLE==RenderMode)	
	{
		return Parent->LL_GetBoneVisible((u16)RMS_boneid);
	}

	for (u32 it=0; it<BonesUsed.size(); it++)
		if (Parent->LL_GetBoneVisible(BonesUsed[it]))	
		{
			return	TRUE;
		}
	return	FALSE;
}


void 	get_pos_bones(const vertBoned1W &v, Fvector& p, CKinematics* Parent )
{
	const Fmatrix& xform	= Parent->LL_GetBoneInstance((u16)v.matrix).mRenderTransform; 
	xform.transform_tiny	( p, v.P );
}

void 	get_pos_bones(const vertBoned2W &vert, Fvector& p, CKinematics* Parent )
{
	Fvector		P0,P1;
	
	Fmatrix& xform0			= Parent->LL_GetBoneInstance( vert.matrix0 ).mRenderTransform; 
	Fmatrix& xform1			= Parent->LL_GetBoneInstance( vert.matrix1 ).mRenderTransform; 
	xform0.transform_tiny	( P0, vert.P );
	xform1.transform_tiny	( P1, vert.P );
	p.lerp					( P0, P1, vert.w );
}



//-----------------------------------------------------------------------------------------------------
// Wallmarks
//-----------------------------------------------------------------------------------------------------
#include "cl_intersect.h"
BOOL	CSkeletonX::_PickBoneSoft1W	(IKinematics::pick_result &r, float dist, const Fvector& S, const Fvector& D, u16* indices, CBoneData::FacesVec& faces)
{
		return pick_bone<vertBoned1W>( Vertices1W, Parent, r, dist, S, D, indices, faces);
}

BOOL CSkeletonX::_PickBoneSoft2W	(IKinematics::pick_result &r, float dist, const Fvector& S, const Fvector& D, u16* indices, CBoneData::FacesVec& faces)
{
		return pick_bone<vertBoned2W>( Vertices2W, Parent,r, dist, S, D, indices, faces);
}

// Fill Vertices
void CSkeletonX::_FillVerticesSoft1W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, u16* indices, CBoneData::FacesVec& faces)
{
	VERIFY				(*Vertices1W);
	for (CBoneData::FacesVecIt it=faces.begin(); it!=faces.end(); it++){
		Fvector			p[3];
		u32 idx			= (*it)*3;
		CSkeletonWallmark::WMFace F;
		for (u32 k=0; k<3; k++){
			vertBoned1W& vert		= Vertices1W[indices[idx+k]];
			F.bone_id[k][0]			= (u16)vert.matrix;
			F.bone_id[k][1]			= F.bone_id[k][0];
			F.weight[k]				= 0.f;
			const Fmatrix& xform	= Parent->LL_GetBoneInstance(F.bone_id[k][0]).mRenderTransform; 
			F.vert[k].set			(vert.P);
			xform.transform_tiny	(p[k],F.vert[k]);
		}
		Fvector test_normal;
		test_normal.mknormal	(p[0],p[1],p[2]);
		float cosa				= test_normal.dotproduct(normal);
		if (cosa<EPS)			continue;
		if (CDB::TestSphereTri(wm.ContactPoint(),size,p))
		{
			Fvector				UV;
			for (u32 k=0; k<3; k++){
				Fvector2& uv	= F.uv[k];
				view.transform_tiny(UV,p[k]);
				uv.x			= (1+UV.x)*.5f;
				uv.y			= (1-UV.y)*.5f;
			}
			wm.m_Faces.push_back(F);
		}
	}
}
void CSkeletonX::_FillVerticesSoft2W(const Fmatrix& view, CSkeletonWallmark& wm, const Fvector& normal, float size, u16* indices, CBoneData::FacesVec& faces)
{
	VERIFY				(*Vertices2W);
	for (CBoneData::FacesVecIt it=faces.begin(); it!=faces.end(); it++){
		Fvector			p[3];
		u32 idx			= (*it)*3;
		CSkeletonWallmark::WMFace F;
		for (u32 k=0; k<3; k++){
			Fvector		P0,P1;
			vertBoned2W& vert		= Vertices2W[indices[idx+k]];
			F.bone_id[k][0]			= vert.matrix0;
			F.bone_id[k][1]			= vert.matrix1;
			F.weight[k]				= vert.w;
			Fmatrix& xform0			= Parent->LL_GetBoneInstance(F.bone_id[k][0]).mRenderTransform; 
			Fmatrix& xform1			= Parent->LL_GetBoneInstance(F.bone_id[k][1]).mRenderTransform; 
			F.vert[k].set			(vert.P);		
			xform0.transform_tiny	(P0,F.vert[k]);
			xform1.transform_tiny	(P1,F.vert[k]);
			p[k].lerp				(P0,P1,F.weight[k]);
		}
		Fvector test_normal;
		test_normal.mknormal	(p[0],p[1],p[2]);
		float cosa				= test_normal.dotproduct(normal);
		if (cosa<EPS)			continue;
		if (CDB::TestSphereTri(wm.ContactPoint(),size,p)){
			Fvector				UV;
			for (u32 k=0; k<3; k++){
				Fvector2& uv	= F.uv[k];
				view.transform_tiny(UV,p[k]);
				uv.x			= (1+UV.x)*.5f;
				uv.y			= (1-UV.y)*.5f;
			}
			wm.m_Faces.push_back(F);
		}
	}
}



