#pragma once

#ifdef SERIALIZATION_SUPPORT

#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>

#endif

START_NAMESPACE_DISTRHO

struct Anchors
{
	enum AnchorType
	{
		None = 1,
		Left = 2,
		Right = 4,
		Top = 8,
		Bottom = 16,
		All = 32
	};

	Anchors();
	Anchors(bool left, bool right, bool top, bool bottom);

	bool left;
	bool right;
	bool top;
	bool bottom;

#ifdef SERIALIZATION_SUPPORT
	template <class Archive>
	void serialize(Archive &ar)
	{
		ar(CEREAL_NVP(left),
		   CEREAL_NVP(right),
		   CEREAL_NVP(top),
		   CEREAL_NVP(bottom));
	}
#endif
};

Anchors::Anchors() : left(true),
					 right(false),
					 top(true),
					 bottom(false)
{
}

Anchors::Anchors(bool left, bool right, bool top, bool bottom) : left(left),
																 right(right),
																 top(top),
																 bottom(bottom)
{
}

struct RelativePosition
{
	RelativePosition();
	RelativePosition(int left, int right, int top, int bottom);

	int left;
	int right;
	int top;
	int bottom;

#ifdef SERIALIZATION_SUPPORT
	template <class Archive>
	void serialize(Archive &ar)
	{
		ar(CEREAL_NVP(left),
		   CEREAL_NVP(right),
		   CEREAL_NVP(top),
		   CEREAL_NVP(bottom));
	}
#endif
};

RelativePosition::RelativePosition() : left(0),
									   right(0),
									   top(0),
									   bottom(0)
{
}

RelativePosition::RelativePosition(int left, int right, int top, int bottom) : left(left),
																			   right(right),
																			   top(top),
																			   bottom(bottom)
{
}

class Layout;

class LayoutItem
{
  public:
	LayoutItem(Layout *parent, Widget *widget);

	Widget *getWidget();

	LayoutItem &setAnchors(int anchors);
	LayoutItem &setAnchors(Anchors anchors);

	Anchors getAnchors();

	LayoutItem &setSize(const uint width, const uint height);
	LayoutItem &setPosition(const int x, const int y);

	void setRelativePos(int left, int right, int top, int bottom);
	void setRelativeLeft(int left);
	void setRelativeRight(int right);
	void setRelativeTop(int top);
	void setRelativeBottom(int bottom);

	RelativePosition getRelativePos();

#ifdef SERIALIZATION_SUPPORT
	template <class Archive>
	void serialize(Archive &ar)
	{
		ar(CEREAL_NVP(fAnchors),
		   CEREAL_NVP(fRelativePos));
	}
#endif
  private:
	Widget *fWidget;
	Layout *fParent;
	Anchors fAnchors;

	// the absolute distance of the item from the sides of the layout
	RelativePosition fRelativePos;
};

LayoutItem::LayoutItem(Layout *parent, Widget *widget) : fWidget(widget),
														 fParent(parent),
														 fAnchors(),
														 fRelativePos()
{
}

Widget *LayoutItem::getWidget()
{
	return fWidget;
}

LayoutItem &LayoutItem::setAnchors(int anchors)
{
	if (anchors & Anchors::All)
	{
		fAnchors.left = true;
		fAnchors.right = true;
		fAnchors.top = true;
		fAnchors.bottom = true;

		return *this;
	}

	fAnchors.left = anchors & Anchors::Left;
	fAnchors.right = anchors & Anchors::Right;
	fAnchors.top = anchors & Anchors::Top;
	fAnchors.bottom = anchors & Anchors::Bottom;

	return *this;
}

LayoutItem &LayoutItem::setAnchors(Anchors anchors)
{
	fAnchors = anchors;

	return *this;
}

Anchors LayoutItem::getAnchors()
{
	return fAnchors;
}

void LayoutItem::setRelativePos(int left, int right, int top, int bottom)
{
	fRelativePos.left = left;
	fRelativePos.right = right;
	fRelativePos.top = top;
	fRelativePos.bottom = bottom;
}

void LayoutItem::setRelativeLeft(int left)
{
	fRelativePos.left = left;
}

void LayoutItem::setRelativeRight(int right)
{
	fRelativePos.right = right;
}

void LayoutItem::setRelativeTop(int top)
{
	fRelativePos.top = top;
}

void LayoutItem::setRelativeBottom(int bottom)
{
	fRelativePos.bottom = bottom;
}

RelativePosition LayoutItem::getRelativePos()
{
	return fRelativePos;
}

class Layout : public Widget
{
  public:
	Layout(Widget *parent);
	LayoutItem &addItem(Widget *widget);
	size_t getItemCount();
	LayoutItem *getItem(const int index);
	LayoutItem *getFirstItem();
	LayoutItem *getLastItem();

  protected:
	virtual void onItemAdded(const LayoutItem &item);
	std::vector<LayoutItem> fItems;
};

Layout::Layout(Widget *parent) : Widget(parent)
{
	setSize(parent->getSize());
}

LayoutItem &LayoutItem::setSize(const uint width, const uint height)
{
	fWidget->setSize(width, height);

	fRelativePos.right = fParent->getWidth() - (fRelativePos.left + fWidget->getWidth());
	fRelativePos.bottom = fParent->getHeight() - (fRelativePos.top + fWidget->getHeight());

	return *this;
}

LayoutItem &LayoutItem::setPosition(const int x, const int y)
{
	fRelativePos.left = x;
	fRelativePos.right = fParent->getWidth() - (fRelativePos.left + fWidget->getWidth());
	fRelativePos.top = y;
	fRelativePos.bottom = fParent->getHeight() - (fRelativePos.top + fWidget->getHeight());

	fWidget->setAbsolutePos(fParent->getAbsoluteX() + fRelativePos.left, fParent->getAbsoluteY() + fRelativePos.top);

	return *this;
}

LayoutItem *Layout::getItem(const int index)
{
	DISTRHO_SAFE_ASSERT_RETURN(index >= 0 && index < getItemCount(), nullptr);

	return &fItems[index];
}

LayoutItem *Layout::getFirstItem()
{
	return getItem(0);
}

LayoutItem *Layout::getLastItem()
{
	return getItem(getItemCount() - 1);
}

size_t Layout::getItemCount()
{
	return fItems.size();
}

LayoutItem &Layout::addItem(Widget *widget)
{
	LayoutItem item = LayoutItem(this, widget);

	fItems.push_back(item);

	onItemAdded(item);

	return fItems[fItems.size() - 1];
}

void Layout::onItemAdded(const LayoutItem &item)
{
}

class RelativeLayout : public Layout
{
  public:
	RelativeLayout(Widget *parent);
	void repositionItems(Size<uint> oldSize, Size<uint> newSize);
	void repositionItems();

#ifdef SERIALIZATION_SUPPORT
	template <class Archive>
	void serialize(Archive &ar)
	{
		ar(CEREAL_NVP(fItems));
	}
#endif
  protected:
	void onResize(const ResizeEvent &ev) override;
	void onItemAdded(const LayoutItem &item) override;
	void onPositionChanged(const PositionChangedEvent &ev) override;
	void onDisplay() override;
};

RelativeLayout::RelativeLayout(Widget *parent) : Layout(parent)
{
	hide();
}

void RelativeLayout::onDisplay()
{
}

void RelativeLayout::repositionItems()
{
	repositionItems(getSize(), getSize());
}

void RelativeLayout::repositionItems(Size<uint> oldSize, Size<uint> newSize)
{
	int absX = getAbsoluteX();
	int absY = getAbsoluteY();

	for (int i = 0; i < getItemCount(); ++i)
	{
		LayoutItem *item = getItem(i);
		const Anchors anchors = item->getAnchors();

		int deltaWidth = newSize.getWidth() - oldSize.getWidth();
		int deltaHeight = newSize.getHeight() - oldSize.getHeight();

		if (!anchors.right)
		{
			int right = item->getRelativePos().right;

			item->setRelativeRight(right + deltaWidth);
		}
		else if (!anchors.left)
		{
			int left = item->getRelativePos().left;

			item->setRelativeLeft(left + deltaWidth);
		}

		item->getWidget()->setAbsoluteX(absX + item->getRelativePos().left);
		item->getWidget()->setWidth(getWidth() - item->getRelativePos().left - item->getRelativePos().right);

		if (!anchors.bottom)
		{
			int bottom = item->getRelativePos().bottom;

			item->setRelativeBottom(bottom + deltaHeight);
		}
		else if (!anchors.top)
		{
			int top = item->getRelativePos().top;

			item->setRelativeTop(top + deltaHeight);
		}

		item->getWidget()->setAbsoluteY(absY + item->getRelativePos().top);
		item->getWidget()->setHeight(getHeight() - item->getRelativePos().top - item->getRelativePos().bottom);
	}
}

void RelativeLayout::onResize(const ResizeEvent &ev)
{
	repositionItems(ev.oldSize, ev.size);
}

void RelativeLayout::onPositionChanged(const PositionChangedEvent &ev)
{
	const int absX = getAbsoluteX();
	const int absY = getAbsoluteY();

	for (int i = 0; i < getItemCount(); ++i)
	{
		LayoutItem *item = getItem(i);

		item->getWidget()->setAbsoluteX(absX + item->getRelativePos().left);
		item->getWidget()->setAbsoluteY(absY + item->getRelativePos().top);
	}
}

void RelativeLayout::onItemAdded(const LayoutItem &item)
{
}

END_NAMESPACE_DISTRHO
