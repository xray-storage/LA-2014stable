#ifndef __UIPDAWND_H__
#define __UIPDAWND_H__
#pragma once

#include "UIDialogWnd.h"
#include "UIPdaAux.h"
#include "../encyclopedia_article_defs.h"

class CInventoryOwner;
class CUIFrameLineWnd;
class CUIButton;
class CUITabControl;
class CUIStatic;
class CUIMapWnd;
class CUIEncyclopediaWnd;
class CUIDiaryWnd;
class CUIActorInfoWnd;
class CUIStalkersRankingWnd;
class CUIEventsWnd;
class CUIPdaContactsWnd;
class CUI3tButton;

 

class CUIPdaWnd: public CUIDialogWnd
{
private:
	typedef CUIDialogWnd	inherited;
protected:
	//�������� ������������� ����������
	CUIFrameLineWnd*		UIMainButtonsBackground;
	CUIFrameLineWnd*		UITimerBackground;

	// ������ PDA
	CUITabControl*			UITabControl;

	// ���������� ������� �����
	void					UpdateDateTime					();
	void					DrawUpdatedSections				();
protected:
	// ���������
	CUIStatic*				UIMainPdaFrame;
	CUIStatic*				m_updatedSectionImage;
	CUIStatic*				m_oldSectionImage;

	// ������� �������� ������
	CUIWindow*				m_pActiveDialog;
	EPdaTabs				m_pActiveSection;
	xr_vector<Fvector2>		m_sign_places_main;

	bool				m_bSkillsEnabled;
	bool				m_bDownloadsEnabled;

public:
	// ���������� PDA
	CUIMapWnd*				UIMapWnd;
	CUIPdaContactsWnd*		UIPdaContactsWnd;
	CUIEncyclopediaWnd*		UIEncyclopediaWnd;
	CUIDiaryWnd*			UIDiaryWnd;
	CUIActorInfoWnd*		UIActorInfo;
	CUIStalkersRankingWnd*	UIStalkersRanking;
	CUIEventsWnd*			UIEventsWnd;
	CUI3tButton*			m_pUIClose;

	virtual void			Reset				();

public:
							CUIPdaWnd			();
	virtual					~CUIPdaWnd			();

	virtual void 			Init				();

	virtual void 			SendMessage			(CUIWindow* pWnd, s16 msg, void* pData = NULL);

	virtual void 			Draw				();
	virtual void 			Update				();
	virtual void 			Show				(bool status);
	
	virtual void 			EnableSkills				(bool val);
	virtual void 			EnableDownloads				(bool val);
	virtual bool			IsSkillsEnabled			(){return m_bSkillsEnabled;}
	virtual bool			IsDownloadsEnabled			(){return m_bDownloadsEnabled;}

	virtual bool			OnMouseAction				(float x, float y, EUIMessages mouse_action) {CUIDialogWnd::OnMouseAction(x,y,mouse_action);return true;} //always true because StopAnyMove() == false
	virtual bool			OnKeyboardAction			(int dik, EUIMessages keyboard_action);
	
	void					SetActiveSubdialog	(EPdaTabs section);
	virtual bool			StopAnyMove			(){return false;}

			void			PdaContentsChanged	(pda_section::part type);
};

#endif