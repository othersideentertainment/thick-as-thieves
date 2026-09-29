#pragma once

#include <Widgets/SWindow.h>

struct ImGuiViewport;

class SImGuiWindow : public SWindow
{
	SLATE_BEGIN_ARGS(SImGuiWindow)
		{
		}

		SLATE_ARGUMENT(ImGuiViewport*, Viewport);
		SLATE_DEFAULT_SLOT(FArguments, Content);
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	virtual bool OnIsActiveChanged(const FWindowActivateEvent& ActivateEvent) override;
};
