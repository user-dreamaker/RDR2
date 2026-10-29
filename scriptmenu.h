#pragma once

#include "script.h"
#include "keyboard.h"

#include <windows.h>
#include <vector>
#include <string>

void DrawText(float x, float y, char *str);
void DrawRect(float lineLeft, float lineTop, float lineWidth, float lineHeight, int r, int g, int b, int a);

class MenuBase;
class MenuController;

struct ColorRgba
{
	byte	r, g, b, a;
};

enum eMenuItemClass
{
	Base,
	Title,
	ListTitle,
	Default,
	Switchable,
	Menu
};

class MenuItemBase
{
	float		m_lineWidth;
	float		m_lineHeight;
	float		m_textLeft;
	ColorRgba	m_colorRect;
	ColorRgba	m_colorText;
	ColorRgba	m_colorRectActive;
	ColorRgba	m_colorTextActive;

	MenuBase *	m_menu;
protected:
	MenuItemBase(
		float lineWidth, float lineHeight, float textLeft,
		ColorRgba colorRect, ColorRgba colorText,
		ColorRgba colorRectActive = {}, ColorRgba colorTextActive = {})
		: m_lineWidth(lineWidth), m_lineHeight(lineHeight), m_textLeft(textLeft),
			m_colorRect(colorRect), m_colorText(colorText),
			m_colorRectActive(colorRectActive), m_colorTextActive(colorTextActive),
			m_menu(nullptr)	{}
	void WaitAndDraw(int ms);
	void SetStatusText(std::string text, int ms = 2500);
public:
	virtual ~MenuItemBase() {}

	virtual eMenuItemClass GetClass() { return eMenuItemClass::Base; }
	virtual void OnDraw(float lineTop, float lineLeft, bool active);
	virtual	void OnSelect() {}
	virtual	std::string GetCaption() { return ""; }

	float GetLineWidth()  { return m_lineWidth;  }
	float GetLineHeight() { return m_lineHeight; }

	ColorRgba GetColorRect() { return m_colorRect; }
	ColorRgba GetColorText() { return m_colorText; }

	ColorRgba GetColorRectActive() { return m_colorRectActive; }
	ColorRgba GetColorTextActive() { return m_colorTextActive; }

	void SetMenu(MenuBase *menu) { m_menu = menu; };
	MenuBase *GetMenu() { return m_menu; };
};

const float
	MenuItemTitle_lineHeight = 0.06f,
	MenuItemTitle_textLeft	 = 0.01f;
extern float MenuItemTitle_lineWidth;

const ColorRgba
	MenuItemTitle_colorRect { 0, 0, 0, 255 },
	MenuItemTitle_colorText { 255, 255, 255, 255 };

class MenuItemTitle : public MenuItemBase
{
	std::string		m_caption;
public:
	MenuItemTitle(std::string caption)
		: MenuItemBase(
				MenuItemTitle_lineWidth, MenuItemTitle_lineHeight, MenuItemTitle_textLeft,
				MenuItemTitle_colorRect, MenuItemTitle_colorText
		  ),
		  m_caption(caption) {}
	virtual eMenuItemClass GetClass() { return eMenuItemClass::Title; }
	virtual	std::string GetCaption() { return m_caption; }
};

class MenuItemListTitle : public MenuItemTitle
{
	int		m_currentItemIndex;
	int		m_itemsTotal;
public:
	MenuItemListTitle(std::string caption)
		: MenuItemTitle(caption),
			m_currentItemIndex(0), m_itemsTotal(0) {}
	virtual eMenuItemClass GetClass() { return eMenuItemClass::ListTitle; }
	virtual	std::string GetCaption() { return MenuItemTitle::GetCaption() + "  " + std::to_string(m_currentItemIndex) + "/" + std::to_string(m_itemsTotal); }
	void SetCurrentItemInfo(int index, int total) { m_currentItemIndex = index, m_itemsTotal = total; }
};

const float
	MenuItemDefault_lineHeight	= 0.05f,
	MenuItemDefault_textLeft	= 0.01f;
extern float MenuItemDefault_lineWidth;

const ColorRgba
	MenuItemDefault_colorRect			{ 60, 60, 60, 180 },
	MenuItemDefault_colorText			{ 255, 255, 255, 150 },
	MenuItemDefault_colorRectActive		{ 140, 140, 140, 220 },
	MenuItemDefault_colorTextActive		{ 0, 0, 0, 200 };

class MenuItemDefault : public MenuItemBase
{
	std::string		m_caption;
public:
	MenuItemDefault(std::string caption)
		: MenuItemBase(
			MenuItemDefault_lineWidth, MenuItemDefault_lineHeight, MenuItemDefault_textLeft,
			MenuItemDefault_colorRect, MenuItemDefault_colorText, MenuItemDefault_colorRectActive, MenuItemDefault_colorTextActive
		  ),
		  m_caption(caption) {}
	virtual eMenuItemClass GetClass() { return eMenuItemClass::Default; }
	virtual	std::string GetCaption() { return m_caption; }
};

class MenuItemSwitchable : public MenuItemDefault
{
	bool	m_state;
public:
	MenuItemSwitchable(std::string caption)
		: MenuItemDefault(caption),
		m_state(false) {}
	virtual eMenuItemClass GetClass() { return eMenuItemClass::Switchable; }
	virtual void OnDraw(float lineTop, float lineLeft, bool active);
	virtual void OnSelect() { m_state = !m_state; }
	void SetState(bool state) { m_state = state; }
	bool GetState() { return m_state; }
};

class MenuItemMenu : public MenuItemDefault
{
	MenuBase *	m_menu;
public:
	MenuItemMenu(std::string caption, MenuBase *menu)
		: MenuItemDefault(caption),
		m_menu(menu) {}
	virtual eMenuItemClass GetClass() { return eMenuItemClass::Menu; }
	virtual void OnDraw(float lineTop, float lineLeft, bool active);
	virtual	void OnSelect();
};

const int
	MenuBase_linesPerScreen = 11;

extern float MenuBase_menuTop;
extern float MenuBase_menuLeft;
const float MenuBase_lineOverlap = 1.0f / 40.0f;

class MenuBase
{
	MenuItemTitle *				m_itemTitle;
	std::vector<MenuItemBase *>	m_items;

	int		m_activeLineIndex;
	int		m_activeScreenIndex;

	MenuController *			m_controller;
public:
	MenuBase(MenuItemTitle *itemTitle)
		: m_itemTitle(itemTitle),
		  m_activeLineIndex(0), m_activeScreenIndex(0),
		  m_controller(nullptr) {}
	~MenuBase()
	{
		delete m_itemTitle;
		for (auto *item : m_items)
			delete item;
	}
	void AddItem(MenuItemBase *item) { item->SetMenu(this); m_items.push_back(item); }
	int GetActiveItemIndex() { return m_activeScreenIndex * MenuBase_linesPerScreen + m_activeLineIndex; }
	void OnDraw();
	int OnInput();
	void SetController(MenuController *controller) { m_controller = controller; }
	MenuController *GetController() { return m_controller; }
};

struct MenuInputButtonState
{
	bool a, b, up, down, l, r;
};

/* configurable key bindings loaded from ini */
extern int g_menuToggle;
extern int g_selectKey;
extern int g_backKey;
extern int g_upKey;
extern int g_downKey;
extern int g_rightKey;
extern int g_leftKey;
extern std::vector<int> g_menuExitKeys;

class MenuInput
{
public:
	static bool MenuSwitchPressed()
	{
		return IsKeyJustUp(g_menuToggle);
	}
	static MenuInputButtonState GetButtonState()
	{
		return {
			IsKeyDown(g_selectKey),
			IsKeyDown(g_backKey) || MenuSwitchPressed(),
			IsKeyDown(g_upKey),
			IsKeyDown(g_downKey),
			IsKeyDown(g_rightKey),
			IsKeyDown(g_leftKey)
		};
	}
	static void MenuInputBeep()
	{
		AUDIO::STOP_SOUND_FRONTEND("NAV_RIGHT", "HUD_SHOP_SOUNDSET");
		AUDIO::PLAY_SOUND_FRONTEND("NAV_RIGHT", "HUD_SHOP_SOUNDSET", 1, 0);
	}
};


class MenuController
{
	std::vector<MenuBase *>		m_menuList;
	std::vector<MenuBase *>		m_menuStack;

	DWORD	m_inputTurnOnTime;

	std::string	m_statusText;
	DWORD	m_statusTextMaxTicks;

	void InputWait(int ms)		{	m_inputTurnOnTime = GetTickCount() + ms; }
	bool InputIsOnWait()		{	return m_inputTurnOnTime > GetTickCount(); }
	MenuBase *GetActiveMenu()	{	return m_menuStack.size() ? m_menuStack[m_menuStack.size() - 1] : nullptr; }
	void DrawStatusText();
	void OnDraw()
	{
		if (auto menu = GetActiveMenu())
			menu->OnDraw();
		DrawStatusText();
	}
	void OnInput()
	{
		if (InputIsOnWait())
			return;
		if (auto menu = GetActiveMenu())
			if (int waitTime = menu->OnInput())
				InputWait(waitTime);
	}
public:
	MenuController()
		: m_inputTurnOnTime(0), m_statusTextMaxTicks(0) {}
	~MenuController()
	{
		for (auto *menu : m_menuList)
			delete menu;
	}
	bool HasActiveMenu()			{	return m_menuStack.size() > 0; }
	void PushMenu(MenuBase *menu)	{	if (IsMenuRegistered(menu)) m_menuStack.push_back(menu); }
	void PopMenu()					{   if (m_menuStack.size()) m_menuStack.pop_back(); }
	void SetStatusText(std::string text, int ms) { m_statusText = text, m_statusTextMaxTicks = GetTickCount() + ms; }
	bool IsMenuRegistered(MenuBase *menu)
	{
		for (size_t i = 0; i < m_menuList.size(); i++)
			if (m_menuList[i] == menu)
				return true;
		return false;
	}
	void RegisterMenu(MenuBase *menu)
	{
		if (!IsMenuRegistered(menu))
		{
			menu->SetController(this);
			m_menuList.push_back(menu);
		}
	}
	void Update()
	{
		OnDraw();
		OnInput();
	}
};
