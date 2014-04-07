#include "stdafx.h"
#include "UIFrameLineWnd.h"
#include "UITextureMaster.h"

CUIFrameLineWnd::CUIFrameLineWnd()
:bHorizontal(true),
m_bTextureVisible(false),
m_bStretchBETextures(false)
{
	m_texture_color				= color_argb(255,255,255,255);
	AttachChild				(&UITitleText);
}

void CUIFrameLineWnd::InitFrameLineWnd(LPCSTR base_name, Fvector2 pos, Fvector2 size, bool horizontal)
{
	InitFrameLineWnd(pos,size,horizontal);
	InitTexture		(base_name,"hud\\default");
}

void CUIFrameLineWnd::InitFrameLineWnd(Fvector2 pos, Fvector2 size, bool horizontal)
{
	inherited::SetWndPos		(pos);
	inherited::SetWndSize		(size);
	
	bHorizontal					= horizontal;

	Frect			rect;
	GetAbsoluteRect	(rect);

	if (horizontal)
	{
		UITitleText.SetWndPos					(Fvector2().set(0.f,0.f));
		UITitleText.SetWndSize					(Fvector2().set(rect.right - rect.left, 50.f)); 
	} else {
		UITitleText.SetWndPos					(Fvector2().set(0.f,0.f));
		UITitleText.SetWndSize					(Fvector2().set(50.f, rect.bottom - rect.top)); 
	}
}

void CUIFrameLineWnd::Init_script(LPCSTR base_name, float x, float y, float width, float height, bool horizontal)
{
	InitFrameLineWnd	(Fvector2().set(x,y),Fvector2().set(width, height),horizontal);
	InitTexture			(base_name,"hud\\default");
}

void CUIFrameLineWnd::InitTexture(LPCSTR texture, LPCSTR sh_name)
{
	m_bTextureVisible			= true;
	dbg_tex_name				= texture;
	string256					buf;
	CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture,"_back"),	sh_name, m_shader, m_tex_rect[flBack]);
	CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture,"_b"),	sh_name, m_shader, m_tex_rect[flFirst]);
	CUITextureMaster::InitTexture(strconcat(sizeof(buf), buf, texture,"_e"),	sh_name, m_shader, m_tex_rect[flSecond]);

	if(bHorizontal)
	{
		R_ASSERT2(fsimilar(m_tex_rect[flFirst].height(), m_tex_rect[flSecond].height()), make_string("CUIFrameLineWnd: '%s_b' and '%s_e' should have different heights, CUIFrameLineWnd is horisontal", texture, texture));
		R_ASSERT2(fsimilar(m_tex_rect[flFirst].height(), m_tex_rect[flBack].height()), make_string("CUIFrameLineWnd: '%s_b' and '%s_back' should have different heights, CUIFrameLineWnd is horisontal", texture, texture));
	} else {
		R_ASSERT2(fsimilar(m_tex_rect[flFirst].width(), m_tex_rect[flSecond].width()), make_string("CUIFrameLineWnd: '%s_b' and '%s_e' should have different widths, CUIFrameLineWnd is vertical", texture, texture));
		R_ASSERT2(fsimilar(m_tex_rect[flFirst].width(), m_tex_rect[flBack].width()),make_string("CUIFrameLineWnd: '%s_b' and '%s_back' should have different widths, CUIFrameLineWnd is vertical", texture, texture));
	}
}

void CUIFrameLineWnd::Draw()
{
	if(m_bTextureVisible)
		DrawElements		();

	inherited::Draw			();
}

static Fvector2 pt_offset		= {-0.5f, -0.5f};

void draw_rect(Fvector2 LTp, Fvector2 RBp, Fvector2 LTt, Fvector2 RBt, u32 clr, Fvector2 const& ts)
{
	UI().AlignPixel			(LTp.x);
	UI().AlignPixel			(LTp.y);
	LTp.add					(pt_offset);
	UI().AlignPixel			(RBp.x);
	UI().AlignPixel			(RBp.y);
	RBp.add					(pt_offset);
	LTt.div					(ts);
	RBt.div					(ts);

	UIRender->PushPoint(LTp.x, LTp.y,	0, clr, LTt.x, LTt.y);
	UIRender->PushPoint(RBp.x, RBp.y,	0, clr, RBt.x, RBt.y);
	UIRender->PushPoint(LTp.x, RBp.y,	0, clr, LTt.x, RBt.y);

	UIRender->PushPoint(LTp.x, LTp.y,	0, clr, LTt.x, LTt.y);
	UIRender->PushPoint(RBp.x, LTp.y,	0, clr, RBt.x, LTt.y);
	UIRender->PushPoint(RBp.x, RBp.y,	0, clr, RBt.x, RBt.y);
}

void CUIFrameLineWnd::DrawElements()
{
	UIRender->SetShader			(*m_shader);

	Fvector2					ts;
	UIRender->GetActiveTextureResolution(ts);

	Frect						rect;
	GetAbsoluteRect				(rect);

	UI().ClientToScreenScaled	(rect.lt);
	UI().ClientToScreenScaled	(rect.rb);
	
	float back_len				= 0.0f;
	u32 prim_count				= 6*2; //first&second 
	Fvector2 scale 				= GetStretchingKoeff();

	if(bHorizontal)
	{
		back_len				= rect.width()-(m_tex_rect[flFirst].width()*scale.x)-(m_tex_rect[flSecond].width()*scale.x);
		if(back_len<0.0f)
			rect.x2				-= back_len;

		if(back_len>0.0f)
			prim_count				+= 6* iCeil(back_len / m_tex_rect[flBack].width()*scale.x);
	}else
	{
		back_len				= rect.height()-(m_tex_rect[flFirst].height()*scale.y)-(m_tex_rect[flSecond].height()*scale.y);
		if(back_len<0)
			rect.y2				-= back_len;

		if(back_len>0.0f)
			prim_count				+= 6* iCeil(back_len / m_tex_rect[flBack].height()*scale.y);
	}

	UIRender->StartPrimitive	(prim_count, IUIRender::ptTriList, UI().m_currentPointType);

	for(int i=0; i<flMax; ++i)
	{
		Fvector2 LTt, RBt;
		Fvector2 LTp, RBp;
		int counter				= 0;

		while(inc_pos(rect, counter, i, LTp, RBp, LTt, RBt))
		{
			draw_rect				(LTp, RBp, LTt, RBt, m_texture_color, ts);
			++counter;
		};
	}
	UIRender->FlushPrimitive		();
}


bool  CUIFrameLineWnd::inc_pos(Frect& rect, int counter, int i, Fvector2& LTp, Fvector2& RBp, Fvector2& LTt, Fvector2& RBt)
{
	Fvector2 scale = GetStretchingKoeff();

	if(i==flFirst || i==flSecond)
	{
		if(counter!=0)	return false;

		LTt				= m_tex_rect[i].lt;
		RBt				= m_tex_rect[i].rb;

		LTp				= rect.lt; 

		RBp				= rect.lt; 
		RBp.x			+= m_tex_rect[i].width()*scale.x;
		RBp.y			+= m_tex_rect[i].height()*scale.y;

	}else //i==flBack
	{
		if(	(bHorizontal && rect.lt.x + (m_tex_rect[flSecond].width()*scale.x)+EPS_L >= rect.rb.x)|| 
			(!bHorizontal && rect.lt.y + (m_tex_rect[flSecond].height()*scale.y)+EPS_L >= rect.rb.y) )
			return false;

		LTt				= m_tex_rect[i].lt;
		LTp				= rect.lt; 

		bool b_draw_reminder = (bHorizontal) ?	(rect.lt.x+m_tex_rect[flBack].width()*scale.x > rect.rb.x-(m_tex_rect[flSecond].width()*scale.x)) :
												(rect.lt.y+m_tex_rect[flBack].height()*scale.y > rect.rb.y-(m_tex_rect[flSecond].height()*scale.y));
		if(b_draw_reminder)
		{ //draw reminder
			float rem_len	= (bHorizontal) ?	rect.rb.x-(m_tex_rect[flSecond].width()*scale.x)-rect.lt.x : 
												rect.rb.y-(m_tex_rect[flSecond].height()*scale.y)-rect.lt.y;

			if(bHorizontal)
			{
				RBt.y			= m_tex_rect[i].rb.y;
				RBt.x			= m_tex_rect[i].lt.x + rem_len;

				RBp				= rect.lt; 
				RBp.x			+= rem_len;
				RBp.y			+= m_tex_rect[i].height();
			}else
			{
				RBt.y			= m_tex_rect[i].lt.y + rem_len;
				RBt.x			= m_tex_rect[i].rb.x;

				RBp				= rect.lt; 
				RBp.x			+= m_tex_rect[i].width();
				RBp.y			+= rem_len;
			}
		}else
		{ //draw full element
			RBt				= m_tex_rect[i].rb;

			RBp				= rect.lt; 
			RBp.x			+= m_tex_rect[i].width();
			RBp.y			+= m_tex_rect[i].height();
		}
	}

	//stretch always
	if(bHorizontal)
		RBp.y			= rect.rb.y;
	else
		RBp.x			= rect.rb.x;

	if(bHorizontal)
		rect.lt.x 		= RBp.x;
	else
		rect.lt.y 		= RBp.y;

	return			true;
}

Fvector2 CUIFrameLineWnd::GetStretchingKoeff()
{
	return m_bStretchBETextures ? Fvector2().set(1.f, 1.f) : UI().GetClientToScreenScaledKoeff();
}