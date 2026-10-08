class PS_RichTextRun
{
	string m_sText;
	string m_sTarget;
	bool m_bBold;
	bool m_bHeader;
	bool m_bBreak;
	bool m_bRule;
	bool m_bGap;
	ref Color m_Color;
}

class PS_RichTextWord
{
	string m_sText;
	ref PS_RichTextRun m_Run;
	int m_iLeadingSpaces;
	bool m_bBreak;
	bool m_bRule;
	bool m_bGap;
	TextWidget m_wText;
	ButtonWidget m_wButton;
	ImageWidget m_wUnderline;
}

/**
 * @brief Invokes local map focus from a briefing link button.
 */
class PS_RichTextLinkHandler : SCR_ButtonBaseComponent
{
	protected string m_sTarget;

	void Setup(string target)
	{
		m_sTarget = target;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != 0)
			return false;

		PS_MapFocusHelper.Focus(m_sTarget);
		super.OnClick(w, x, y, button);
		return true;
	}
}

/**
 * @brief Builds a wrapped widget flow for the briefing-only markup.
 */
class PS_RichTextFlowRenderer
{
	protected ref array<ref PS_RichTextRun> m_aRuns = {};
	protected ref array<ref PS_RichTextWord> m_aWords = {};
	protected ref array<ref PS_RichTextLinkHandler> m_aHandlers = {};
	protected SizeLayoutWidget m_wSize;
	protected FrameWidget m_wFrame;
	protected Widget m_wParent;
	protected float m_fMaxWidth;
	protected int m_iPendingSpaces;
	protected bool m_bBold;
	protected bool m_bHeader;
	protected string m_sTarget;
	protected ref Color m_Color;

	void Render(Widget container, string markup, float maxWidth)
	{
		Clear();
		if (!container || !GetGame())
			return;

		m_wParent = container;
		m_fMaxWidth = maxWidth;
		WorkspaceWidget workspace = GetGame().GetWorkspace();
		m_wSize = SizeLayoutWidget.Cast(workspace.CreateWidget(WidgetType.SizeLayoutWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.INHERIT_CLIPPING, Color.White, 0, container));
		if (!m_wSize)
			return;

		AlignableSlot.SetHorizontalAlign(m_wSize, LayoutHorizontalAlign.Stretch);
		AlignableSlot.SetVerticalAlign(m_wSize, LayoutVerticalAlign.Top);
		m_wSize.EnableWidthOverride(true);
		m_wSize.EnableHeightOverride(true);
		m_wSize.SetWidthOverride(Math.Max(1, maxWidth));
		m_wSize.SetHeightOverride(1);
		m_wFrame = FrameWidget.Cast(workspace.CreateWidget(WidgetType.FrameWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.INHERIT_CLIPPING, Color.White, 0, m_wSize));
		if (!m_wFrame)
		{
			Clear();
			return;
		}
		AlignableSlot.SetHorizontalAlign(m_wFrame, LayoutHorizontalAlign.Stretch);
		AlignableSlot.SetVerticalAlign(m_wFrame, LayoutVerticalAlign.Top);

		Parse(markup);
		BuildWords();
		GetGame().GetCallqueue().CallLater(DoLayout, 0);
	}

	void Clear()
	{
		if (GetGame())
			GetGame().GetCallqueue().Remove(DoLayout);

		if (m_wSize)
			m_wSize.RemoveFromHierarchy();
		m_wSize = null;
		m_wFrame = null;
		m_wParent = null;
		m_aHandlers.Clear();
		m_aWords.Clear();
		m_aRuns.Clear();
		m_iPendingSpaces = 0;
	}

	static string StripUnknownTags(string markup)
	{
		if (!markup.Contains("<"))
			return markup;

		string output = "";
		for (int i = 0; i < markup.Length(); i++)
		{
			string current = markup.Substring(i, 1);
			if (current != "<")
			{
				output += current;
				continue;
			}

			string rest = markup.Substring(i, markup.Length() - i);
			int close = rest.IndexOf(">");
			if (close < 0)
			{
				output += current;
				continue;
			}

			string tag = rest.Substring(1, close - 1).Trim();
			if (IsVanillaRichTextTag(tag))
				output += rest.Substring(0, close + 1);
			i += close;
		}
		return output;
	}

	protected static bool IsVanillaRichTextTag(string tag)
	{
		string name = tag;
		if (!name.IsEmpty() && name.Substring(0, 1) == "/")
			name = name.Substring(1, name.Length() - 1);

		int space = name.IndexOf(" ");
		if (space >= 0)
			name = name.Substring(0, space);
		if (!name.IsEmpty() && name.Substring(name.Length() - 1, 1) == "/")
			name = name.Substring(0, name.Length() - 1);

		return name == "b" || name == "i" || name == "color" || name == "font" || name == "outline" || name == "shadow" || name == "image" || name == "p" || name == "h" || name == "h1" || name == "h2" || name == "ucs" || name == "action" || name == "key" || name == "br";
	}

	protected void AddControl(bool isBreak, bool isRule, bool isGap)
	{
		PS_RichTextRun run = new PS_RichTextRun();
		run.m_bBreak = isBreak;
		run.m_bRule = isRule;
		run.m_bGap = isGap;
		m_aRuns.Insert(run);
	}

	protected void AddText(string value)
	{
		if (value.IsEmpty())
			return;

		PS_RichTextRun run = new PS_RichTextRun();
		run.m_sText = value;
		run.m_sTarget = m_sTarget;
		run.m_bBold = m_bBold || m_bHeader;
		run.m_bHeader = m_bHeader;
		run.m_Color = m_Color;
		m_aRuns.Insert(run);
	}

	protected void Parse(string markup)
	{
		m_bBold = false;
		m_bHeader = false;
		m_sTarget = "";
		m_Color = Color.FromSRGBA(230, 230, 230, 255);
		string buffer = "";

		for (int i = 0; i < markup.Length(); i++)
		{
			string current = markup.Substring(i, 1);
			if (current != "<")
			{
				buffer += current;
				continue;
			}

			string rest = markup.Substring(i, markup.Length() - i);
			int close = rest.IndexOf(">");
			if (close < 0)
			{
				buffer += current;
				continue;
			}

			AddText(buffer);
			buffer = "";
			string tag = rest.Substring(1, close - 1);
			i += close;

			if (tag == "h")
			{
				if (!m_aRuns.IsEmpty())
				{
					PS_RichTextRun previous = m_aRuns.Get(m_aRuns.Count() - 1);
					if (!previous.m_bBreak && !previous.m_bRule && !previous.m_bGap)
						AddControl(true, false, false);
				}
				m_bHeader = true;
			}
			else if (tag == "/h")
			{
				m_bHeader = false;
				AddControl(true, false, false);
			}
			else if (tag == "b")
				m_bBold = true;
			else if (tag == "/b")
				m_bBold = false;
			else if (tag == "br" || tag == "br/")
				AddControl(true, false, false);
			else if (tag == "hr" || tag == "hr/")
				AddControl(false, true, false);
			else if (tag == "gap" || tag == "gap/")
				AddControl(false, false, true);
			else if (tag == "/link")
				m_sTarget = "";
			else if (tag.IndexOf("link=") == 0)
				m_sTarget = tag.Substring(5, tag.Length() - 5);
			else if (tag == "/color")
				m_Color = Color.FromSRGBA(230, 230, 230, 255);
			else if (tag.IndexOf("color=") == 0)
				m_Color = ParseColor(tag.Substring(6, tag.Length() - 6));
			// Unknown tags are intentionally omitted from the visible text.
		}

		AddText(buffer);
	}

	protected Color ParseColor(string colorSpec)
	{
		if (colorSpec.Length() == 7 || colorSpec.Length() == 9)
		{
			if (colorSpec.Substring(0, 1) == "#")
			{
				int red, green, blue, alpha = 255;
				if (TryHexByte(colorSpec, 1, red) && TryHexByte(colorSpec, 3, green) && TryHexByte(colorSpec, 5, blue))
				{
					if (colorSpec.Length() == 7 || TryHexByte(colorSpec, 7, alpha))
						return Color.FromSRGBA(red, green, blue, alpha);
				}
			}
		}

		ref array<string> values = {};
		colorSpec.Split(",", values, true);
		if (values.Count() == 3 || values.Count() == 4)
		{
			int red, green, blue, alpha = 255;
			if (TryByte(values[0], red) && TryByte(values[1], green) && TryByte(values[2], blue))
			{
				if (values.Count() == 3 || TryByte(values[3], alpha))
					return Color.FromSRGBA(red, green, blue, alpha);
			}
		}

		return Color.FromSRGBA(230, 230, 230, 255);
	}

	protected bool TryByte(string value, out int result)
	{
		string number = value.Trim();
		int parsed;
		result = number.ToInt(parsed: parsed);
		return parsed == number.Length() && parsed > 0 && result >= 0 && result <= 255;
	}

	protected bool TryHexByte(string value, int start, out int result)
	{
		int high = HexDigit(value.ToAscii(start));
		int low = HexDigit(value.ToAscii(start + 1));
		result = high * 16 + low;
		return high >= 0 && low >= 0;
	}

	protected int HexDigit(int ascii)
	{
		if (ascii >= 48 && ascii <= 57)
			return ascii - 48;
		if (ascii >= 65 && ascii <= 70)
			return ascii - 55;
		if (ascii >= 97 && ascii <= 102)
			return ascii - 87;
		return -1;
	}

	protected void BuildWords()
	{
		foreach (PS_RichTextRun run : m_aRuns)
		{
			if (run.m_bBreak || run.m_bRule || run.m_bGap)
			{
				PS_RichTextWord control = new PS_RichTextWord();
				control.m_bBreak = run.m_bBreak;
				control.m_bRule = run.m_bRule;
				control.m_bGap = run.m_bGap;
				m_aWords.Insert(control);
				m_iPendingSpaces = 0;
				continue;
			}

			string wordText = "";
			for (int i = 0; i < run.m_sText.Length(); i++)
			{
				string character = run.m_sText.Substring(i, 1);
				if (character == " " || character == "\t" || character == "\n" || character == "\r")
				{
					if (!wordText.IsEmpty())
					{
						AddWord(wordText, run);
						wordText = "";
					}

					if (character == "\n")
					{
						if (!m_aWords.IsEmpty())
						{
							PS_RichTextWord previous = m_aWords.Get(m_aWords.Count() - 1);
							if (!previous.m_bBreak && !previous.m_bRule && !previous.m_bGap)
							{
								PS_RichTextWord lineBreak = new PS_RichTextWord();
								lineBreak.m_bBreak = true;
								m_aWords.Insert(lineBreak);
							}
						}
						m_iPendingSpaces = 0;
					}
					else if (character == " " || character == "\t")
						m_iPendingSpaces++;
				}
				else
					wordText += character;
			}

			if (!wordText.IsEmpty())
				AddWord(wordText, run);
		}
	}

	protected void AddWord(string text, PS_RichTextRun run)
	{
		PS_RichTextWord word = CreateWord(text, run, m_iPendingSpaces);
		m_iPendingSpaces = 0;
		m_aWords.Insert(word);
	}

	protected PS_RichTextWord CreateWord(string text, PS_RichTextRun run, int leadingSpaces)
	{
		PS_RichTextWord word = new PS_RichTextWord();
		word.m_sText = text;
		word.m_Run = run;
		word.m_iLeadingSpaces = leadingSpaces;
		string displayText = text;
		for (int i = 0; i < word.m_iLeadingSpaces; i++)
			displayText = " " + displayText;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		word.m_wText = TextWidget.Cast(workspace.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.INHERIT_CLIPPING | WidgetFlags.IGNORE_CURSOR, Color.White, 0, m_wFrame));
		word.m_wText.SetText(displayText);
		if (run.m_bHeader)
			word.m_wText.SetExactFontSize(23);
		else
			word.m_wText.SetExactFontSize(20);
		word.m_wText.SetBold(run.m_bBold);
		if (run.m_sTarget.IsEmpty())
			word.m_wText.SetColor(run.m_Color);
		else
			word.m_wText.SetColor(Color.FromSRGBA(237, 153, 61, 255));
		FrameSlot.SetSizeToContent(word.m_wText, true);

		if (!run.m_sTarget.IsEmpty())
		{
			word.m_wUnderline = ImageWidget.Cast(workspace.CreateWidget(WidgetType.ImageWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.INHERIT_CLIPPING | WidgetFlags.STRETCH, Color.FromSRGBA(237, 153, 61, 255), 1, m_wFrame));
			word.m_wButton = ButtonWidget.Cast(workspace.CreateWidget(WidgetType.ButtonWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.INHERIT_CLIPPING, new Color(1, 1, 1, 0), 2, m_wFrame));
			PS_RichTextLinkHandler handler = new PS_RichTextLinkHandler();
			handler.Setup(run.m_sTarget);
			word.m_wButton.AddHandler(handler);
			m_aHandlers.Insert(handler);
		}

		return word;
	}

	protected void SplitWideWords(WorkspaceWidget workspace)
	{
		ref array<ref PS_RichTextWord> wrappedWords = {};
		foreach (PS_RichTextWord word : m_aWords)
		{
			if (word.m_bBreak || word.m_bRule || word.m_bGap)
			{
				wrappedWords.Insert(word);
				continue;
			}

			float width, height;
			word.m_wText.GetScreenSize(width, height);
			width = workspace.DPIUnscale(width);
			if (width <= 0)
			{
				word.m_wText.GetTextSize(width, height);
				width = workspace.DPIUnscale(width);
			}
			if (width <= m_fMaxWidth || word.m_sText.Length() < 2)
			{
				wrappedWords.Insert(word);
				continue;
			}

			string remaining = word.m_sText;
			string firstFragment = "";
			bool isFirst = true;
			while (!remaining.IsEmpty())
			{
				int fragmentLength = FindFittingPrefix(word.m_wText, remaining, workspace);
				string fragment = remaining.Substring(0, fragmentLength);
				if (isFirst)
				{
					firstFragment = fragment;
					word.m_sText = fragment;
					word.m_iLeadingSpaces = 0;
					wrappedWords.Insert(word);
					isFirst = false;
				}
				else
					wrappedWords.Insert(CreateWord(fragment, word.m_Run, 0));

				remaining = remaining.Substring(fragmentLength, remaining.Length() - fragmentLength);
			}
			word.m_wText.SetText(firstFragment);
		}

		m_aWords.Clear();
		foreach (PS_RichTextWord wrappedWord : wrappedWords)
			m_aWords.Insert(wrappedWord);
	}

	protected int FindFittingPrefix(TextWidget textWidget, string text, WorkspaceWidget workspace)
	{
		int low = 1;
		int high = text.Length();
		int best = 0;
		while (low <= high)
		{
			int middle = (low + high) / 2;
			textWidget.SetText(text.Substring(0, middle));
			float width, height;
			textWidget.GetTextSize(width, height);
			width = workspace.DPIUnscale(width);
			if (width <= m_fMaxWidth)
			{
				best = middle;
				low = middle + 1;
			}
			else
				high = middle - 1;
		}

		if (best < 1)
			return 1;
		return best;
	}

	// Widgets are measured on the next UI tick so font metrics and DPI are available.
	protected void DoLayout()
	{
		if (!m_wSize || !m_wFrame || !m_wParent || !GetGame())
			return;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		float screenWidth, screenHeight;
		m_wParent.GetScreenSize(screenWidth, screenHeight);
		if (screenWidth > 0)
			m_fMaxWidth = workspace.DPIUnscale(screenWidth) - 16;
		m_fMaxWidth = Math.Max(1, m_fMaxWidth);
		m_wSize.SetWidthOverride(m_fMaxWidth);
		SplitWideWords(workspace);

		float x = 0;
		float y = 0;
		float lineHeight = 24;
		foreach (PS_RichTextWord word : m_aWords)
		{
			if (word.m_bBreak)
			{
				x = 0;
				y += lineHeight;
				lineHeight = 24;
				continue;
			}

			if (word.m_bRule || word.m_bGap)
			{
				if (x > 0)
					y += lineHeight;
				x = 0;
				if (word.m_bRule)
					y += 12;
				else
					y += 8;
				lineHeight = 24;
				if (word.m_bRule)
				{
					ImageWidget divider = ImageWidget.Cast(workspace.CreateWidget(WidgetType.ImageWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.STRETCH, Color.FromSRGBA(140, 140, 140, 255), 0, m_wFrame));
					FrameSlot.SetPos(divider, 0, y - 6);
					FrameSlot.SetSize(divider, m_fMaxWidth, 1);
				}
				continue;
			}

			float width, height;
			word.m_wText.GetScreenSize(width, height);
			width = workspace.DPIUnscale(width);
			height = workspace.DPIUnscale(height);
			if (width <= 0 || height <= 0)
			{
				word.m_wText.GetTextSize(width, height);
				width = workspace.DPIUnscale(width);
				height = workspace.DPIUnscale(height);
			}

			if (x > 0 && x + width > m_fMaxWidth)
			{
				x = 0;
				y += lineHeight;
				lineHeight = 24;
				if (word.m_iLeadingSpaces > 0)
				{
					word.m_wText.SetText(word.m_sText);
					word.m_wText.GetTextSize(width, height);
					width = workspace.DPIUnscale(width);
					height = workspace.DPIUnscale(height);
				}
			}

			FrameSlot.SetPos(word.m_wText, x, y);
			if (word.m_wButton)
			{
				FrameSlot.SetPos(word.m_wButton, x, y);
				FrameSlot.SetSize(word.m_wButton, width, height);
				FrameSlot.SetPos(word.m_wUnderline, x, y + height - 2);
				FrameSlot.SetSize(word.m_wUnderline, width, 1);
			}
			x += width;
			lineHeight = Math.Max(lineHeight, height);
		}

		m_wSize.SetHeightOverride(y + lineHeight);
	}
}
