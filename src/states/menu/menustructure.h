#pragma once
#include <functional>
#include <vector>
#include <list>
#include <unordered_map>

#include "luminoveau.h"

#include "utils/vectors.h"
#include "utils/colors.h"

#define NPATCH 20

class menuobject
{
private:
	int npatch = NPATCH;

public:
	menuobject()
	{
		sName = "root";
		internalName = "empty";
	}

	menuobject& setActive(bool active)
	{
		isActive = active;
		return *this;
	}

	menuobject(const std::string& name) : sName(name) {}


	menuobject& SetTable(int nColumns, int nRows)
	{
		cellTable = { nColumns, nRows };
		return *this;
	}

	std::string GetName()
	{
		return sName;
	}


	menuobject& SetName(std::string name)
	{
		sName = name;
		return *this;
	}

	std::string GetInternalName()
	{
		if (internalName.empty())
			return "empty";

		return internalName;
	}

	menuobject& SetInternalName(std::string name)
	{
		internalName = name;
		return *this;
	}

	bool HasChildren()
	{
		return !items.empty();
	}

	// For now, cells are simply one line strings
	vi2d GetSize()
	{

        return  { static_cast<int>(sName.length() * 10), 1};
        //TODO: font measure text
		//return { 2 + MeasureText(sName.c_str(), npatch) / (int)npatch, 1 };
	}

	vi2d& GetCursorPosition()
	{
		return vCursorPos;
	}

	menuobject& operator[](const char* name)
	{
		if (itemPointer.count(name) == 0)
		{
			itemPointer[name] = items.size();
			items.emplace_back(menuobject(name));
		}

		return items[itemPointer[name]];
	}

	void clearActive()
	{
		for (auto& item : items)
			item.setActive(false);
	}

	void Build()
	{
		for (auto& m : items)
		{
			if (m.HasChildren())
			{
				m.Build();
			}

			cellSize.x = std::max(m.GetSize().x, cellSize.x);
			cellSize.y = std::max(m.GetSize().y, cellSize.y);
		}

		sizeInpatches.x = cellTable.x * cellSize.x + (cellTable.x - 1) * cellPadding.x + 2;
		sizeInpatches.y = cellTable.y * cellSize.y + (cellTable.y - 1) * cellPadding.y + 2;

		totalRows = (static_cast<int32_t>(items.size()) / cellTable.x) + (((items.size() % cellTable.x) > 0) ? 1 : 0);
	}

	menuobject& setCallback(std::function<void()> _c)
	{
		callback = _c;
		return *this;
	}

	void DrawSelf(const vi2d &screenOffset)
	{
		vi2d patchPos = { 0,0 };
		for (patchPos.x = 0; patchPos.x < sizeInpatches.x; patchPos.x++)
		{
			for (patchPos.y = 0; patchPos.y < sizeInpatches.y; patchPos.y++)
			{
				//vi2d screenLocation = patchPos * npatch + screenOffset;

				// Calculate which patch is needed
				//vi2d sourcePatch = { 0, 0 };
//				if (patchPos.x > 0) sourcePatch.x = 1;
//				if (patchPos.x == sizeInpatches.x - 1) sourcePatch.x = 2;
//				if (patchPos.y > 0) sourcePatch.y = 1;
//				if (patchPos.y == sizeInpatches.y - 1) sourcePatch.y = 2;
			}
		}

		// === Draw Panel Contents
		vi2d cell = { 0,0 };
		patchPos = { 1,1 };

		// Work out visible items
		const int32_t topLeftItem = topVisibleRow * cellTable.x;
		int32_t bottomRightItem = cellTable.y * cellTable.x + topLeftItem;

		// Clamp to size of child item vector
		bottomRightItem = std::min(int32_t(items.size()), bottomRightItem);
		const int32_t invisibleItems = bottomRightItem - topLeftItem;

		// Draw Scroll Markers (if required)
		if (topVisibleRow > 0)
		{
			patchPos = { sizeInpatches.x - 2, 0 };
			//vi2d screenLocation = patchPos * npatch + screenOffset;
			//vi2d sourcePatch = { 3, 0 };
			//pge.DrawPartialSprite(screenLocation, sprGFX, sourcePatch * npatch, vPatchSize);
		}

		if ((totalRows - topVisibleRow) > cellTable.y)
		{
			patchPos = { sizeInpatches.x - 2, sizeInpatches.y - 1 };
			//vi2d screenLocation = patchPos * npatch + screenOffset;
			//vi2d sourcePatch = { 3, 2 };
			//pge.DrawPartialSprite(screenLocation, sprGFX, sourcePatch * npatch, vPatchSize);
		}

		// Draw Visible Items
		for (int32_t i = 0; i < invisibleItems; i++)
		{
			// Cell location
			cell.x = i % cellTable.x;
			cell.y = i / cellTable.x;

			// Patch location (including border offset and padding)
			patchPos.x = cell.x * (cellSize.x + cellPadding.x) + 1;
			patchPos.y = cell.y * (cellSize.y + cellPadding.y) + 1;

			// Actual screen location in pixels
			vi2d screenLocation = patchPos * npatch + screenOffset;

			// Display Item Header
			//pge.DrawString(screenLocation, items[topLeftItem + i].sName, items[topLeftItem + i].bEnabled ? olc::WHITE : olc::DARK_GREY);


			std::string text = items[topLeftItem + i].sName;

			if (items[topLeftItem + i].isActive)
				text.append(" *");


			Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {static_cast<float>(screenLocation.x), static_cast<float>(screenLocation.y)}, text.c_str(), WHITE);

			if (items[topLeftItem + i].HasChildren())
			{
				// Display Indicator that panel has a sub panel
				patchPos.x = cell.x * (cellSize.x + cellPadding.x) + 1 + cellSize.x;
				patchPos.y = cell.y * (cellSize.y + cellPadding.y) + 1;
				//vi2d sourcePatch = { 3, 1 };
				screenLocation = patchPos * npatch + screenOffset;
				//pge.DrawPartialSprite(screenLocation, sprGFX, sourcePatch * npatch, vPatchSize);

                Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {static_cast<float>(screenLocation.x), static_cast<float>(screenLocation.y)}, text.c_str(), WHITE);

			}
		}

		// Calculate cursor position in screen space in case system draws it
		vCursorPos.x = (cellCursor.x * (cellSize.x + cellPadding.x)) * npatch + screenOffset.x - npatch;
		vCursorPos.y = ((cellCursor.y - topVisibleRow) * (cellSize.y + cellPadding.y)) * npatch + screenOffset.y + npatch;
	}

	void ClampCursor()
	{
		// Find item in children
		cursorItem = cellCursor.y * cellTable.x + cellCursor.x;

		// Clamp Cursor
		if (cursorItem >= static_cast<int32_t>(items.size()))
		{
			cellCursor.y = (static_cast<int>(items.size()) / cellTable.x);
			cellCursor.x = (static_cast<int>(items.size()) % cellTable.x) - 1;
			cursorItem = static_cast<int32_t>(items.size()) - 1;
		}
	}

	void OnUp()
	{
		cellCursor.y--;
		if (cellCursor.y < 0) cellCursor.y = 0;

		if (cellCursor.y < topVisibleRow)
		{
			topVisibleRow--;
			if (topVisibleRow < 0) topVisibleRow = 0;
		}

		ClampCursor();
	}

	void OnDown()
	{
		cellCursor.y++;
		if (cellCursor.y == totalRows) cellCursor.y = totalRows - 1;

		if (cellCursor.y > (topVisibleRow + cellTable.y - 1))
		{
			topVisibleRow++;
			if (topVisibleRow > (totalRows - cellTable.y))
				topVisibleRow = totalRows - cellTable.y;
		}

		ClampCursor();
	}

	void OnLeft()
	{
		cellCursor.x--;
		if (cellCursor.x < 0) cellCursor.x = 0;
		ClampCursor();
	}

	void OnRight()
	{
		cellCursor.x++;
		if (cellCursor.x == cellTable.x) cellCursor.x = cellTable.x - 1;
		ClampCursor();
	}

	menuobject* OnConfirm()
	{
		// Check if selected item has children
		if (items[cursorItem].HasChildren())
		{
			return &items[cursorItem];
		}
		else
			return this;
	}

	menuobject* GetSelectedItem()
	{
		return &items[cursorItem];
	}


	menuobject& callBack()
	{
		callback();
		return *this;
	}


protected:
	
	vi2d cellTable = { 1, 0 };
	std::unordered_map<std::string, size_t> itemPointer;
	std::vector<menuobject> items;
	bool isActive = false;

	std::string internalName;


	vi2d sizeInpatches = { 0, 0 };
	vi2d cellSize = { 0, 0 };
	vi2d cellPadding = { 2, 0 };
	vi2d cellCursor = { 0, 0 };
	int32_t cursorItem = 0;
	int32_t topVisibleRow = 0;
	int32_t totalRows = 0;
	vi2d vPatchSize = { 8, 8 };
	std::string sName;
	vi2d vCursorPos = { 0, 0 };
	std::function<void()> callback = []() { return 0; };
};


class menumanager
{
public:
	menumanager() = default;

	void Open(menuobject* mo) { Close(); panels.push_back(mo); }
	void Close() { panels.clear(); }

	void OnUp() { if (!panels.empty()) panels.back()->OnUp(); }
	void OnDown() { if (!panels.empty()) panels.back()->OnDown(); }
	void OnLeft() { if (!panels.empty()) panels.back()->OnLeft(); }
	void OnRight() { if (!panels.empty()) panels.back()->OnRight(); }
	void OnBack() { if (!panels.empty() && panels.size() > 1) panels.pop_back(); }
	menuobject* OnConfirm()
	{
		if (panels.empty()) return nullptr;

		menuobject* next = panels.back()->OnConfirm();
		if (next == panels.back())
		{
			return panels.back()->GetSelectedItem();
		}
		else
		{
			panels.push_back(next);
		}

		return nullptr;
	}

	void Draw(vi2d screenOffset)
	{
		if (panels.empty()) return;

		// Draw Visible Menu System
		for (auto& p : panels)
		{
			p->DrawSelf(screenOffset);
			screenOffset += {200, 0};
		}

        Text::DrawText(AssetHandler::GetFont("assets/fonts/APL386.ttf", 18), {static_cast<float>(panels.back()->GetCursorPosition().x), static_cast<float>(panels.back()->GetCursorPosition().y)}, "->", WHITE);
	}

private:
	std::list<menuobject*> panels;
};
