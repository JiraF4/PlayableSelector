class PS_RichTextRun
{
	string m_sText;
	bool m_bBreak;
	bool m_bRule;
	bool m_bGap;
	string m_sFocusName;
	bool m_bBold;
	int m_iColor;
	int m_iFontSize;
}

class PS_RichTextWord
{
	Widget m_wText;
	Widget m_wButton;
	Widget m_wUnderline;
	bool m_bBreak;
	bool m_bRule;
	bool m_bGap;
	bool m_bLinkCont;
	int m_iLeadingSpaces;
}

class PS_RichTextLinkHandler : ScriptedWidgetComponent
{
	protected string m_sFocusName;

	void Setup(string focusName)
	{
		m_sFocusName = focusName;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (button != 0)
			return false;

		PS_MapFocusHelper.Focus(m_sFocusName);
		return true;
	}
}

/**
 * @brief Minimal rich-text renderer with clickable links for briefing screens.
 * Aligned with lite-lobby-ar tested engine pattern.
 */
class PS_RichTextFlowRenderer
{
	protected static const ResourceName FONT = "{3E7733BAC8C831F6}UI/Fonts/RobotoCondensed/RobotoCondensed_Regular.fnt";
	protected static const int FONT_SIZE = 20;
	protected static const int HEADER_FONT_SIZE = 23;
	protected static const float SPACE = 6;
	protected static const float FALLBACK_LINE_HEIGHT = 24;
	protected static const float UNDERLINE_THICKNESS = 2;
	protected static const float UNDERLINE_GAP = 3;

	protected static const float DIVIDER_THICKNESS = 1;
	protected static const float DIVIDER_ROW_HEIGHT = 28;
	protected static const float DIVIDER_PAD = 14;
	protected static const float GAP_HEIGHT = 8;

	// ARGB, rendered through Color.FromSRGBA so widget colours match RichText's
	// <color rgba> path; FromInt skips the sRGB curve and washes mid tones.
	protected static const int COL_TEXT = 0xFFE6E6E6;
	protected static const int COL_HEADER = 0xFFFFFFFF;
	protected static const int COL_LINK = 0xFFE2A74F;
	protected static const int COL_DIVIDER = 0x3CE2A74F;

	// Keeps each pending instance alive until its deferred layout runs.
	protected static ref array<ref PS_RichTextFlowRenderer> s_Pending = {};

	protected static const int MAX_LAYOUT_TRIES = 30;

	protected Widget m_wFrame;
	protected float m_fMaxWidth;
	protected ref array<ref PS_RichTextWord> m_aWords = {};
	protected int m_iLayoutTries;
	protected float m_fLineHeight = FALLBACK_LINE_HEIGHT;

	static PS_RichTextFlowRenderer Render(notnull Widget container, string markup, float maxWidth)
	{
		PS_RichTextFlowRenderer inst = new PS_RichTextFlowRenderer();
		s_Pending.Insert(inst);
		inst.Build(container, markup, maxWidth);
		return inst;
	}

	void Clear()
	{
		s_Pending.RemoveItem(this);
		if (GetGame())
			GetGame().GetCallqueue().Remove(DoLayout);
		if (m_wFrame)
		{
			m_wFrame.RemoveFromHierarchy();
			m_wFrame = null;
		}
		m_aWords.Clear();
	}

	protected static Color SRGB(int argb)
	{
		int a = (argb >> 24) & 0xFF;
		int r = (argb >> 16) & 0xFF;
		int g = (argb >> 8) & 0xFF;
		int b = argb & 0xFF;
		return Color.FromSRGBA(r, g, b, a);
	}

	void Build(Widget container, string markup, float maxWidth)
	{
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws || !container)
			return;

		m_wFrame = ws.CreateWidget(WidgetType.FrameWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.INHERIT_CLIPPING, Color.FromInt(0x00000000), 0, container);
		m_fMaxWidth = maxWidth;
		if (!m_wFrame)
			return;

		AlignableSlot.SetHorizontalAlign(m_wFrame, LayoutHorizontalAlign.Stretch);
		AlignableSlot.SetVerticalAlign(m_wFrame, LayoutVerticalAlign.Top);

		array<ref PS_RichTextRun> runs = {};
		Parse(markup, runs);
		CreateWords(ws, runs);

		GetGame().GetCallqueue().CallLater(DoLayout, 0, false);
	}

	protected static int HexDigit(int ascii)
	{
		if (ascii >= 48 && ascii <= 57)
			return ascii - 48;
		if (ascii >= 65 && ascii <= 70)
			return ascii - 55;
		if (ascii >= 97 && ascii <= 102)
			return ascii - 87;
		return -1;
	}

	protected static int HexToByte(string hex)
	{
		int hi = HexDigit(hex.ToAscii(0));
		int lo = HexDigit(hex.ToAscii(1));
		if (hi < 0 || lo < 0)
			return 255;
		return hi * 16 + lo;
	}

	protected static int ParseColorSpec(string spec)
	{
		string s = spec.Trim();
		if (s.StartsWith("#"))
		{
			s = s.Substring(1, s.Length() - 1);
			if (s.Length() == 6)
			{
				int r = HexToByte(s.Substring(0, 2));
				int g = HexToByte(s.Substring(2, 2));
				int b = HexToByte(s.Substring(4, 2));
				return (0xFF << 24) | (r << 16) | (g << 8) | b;
			}
			else if (s.Length() == 8)
			{
				int a = HexToByte(s.Substring(0, 2));
				int r = HexToByte(s.Substring(2, 2));
				int g = HexToByte(s.Substring(4, 2));
				int b = HexToByte(s.Substring(6, 2));
				return (a << 24) | (r << 16) | (g << 8) | b;
			}
		}

		array<string> parts = {};
		spec.Split(",", parts, true);
		int r = 255, g = 255, b = 255, a = 255;
		if (parts.Count() >= 3)
		{
			r = parts[0].ToInt();
			g = parts[1].ToInt();
			b = parts[2].ToInt();
		}
		if (parts.Count() >= 4)
			a = parts[3].ToInt();
		return (a << 24) | (r << 16) | (g << 8) | b;
	}

	protected void Parse(string markup, notnull array<ref PS_RichTextRun> runs)
	{
		bool bold = false;
		bool header = false;
		string focusName = "";
		int color = COL_TEXT;
		ref array<int> colorStack = {};

		string s = markup;
		while (s.Length() > 0)
		{
			int currentFontSize = FONT_SIZE;
			if (header)
				currentFontSize = HEADER_FONT_SIZE;

			int currentColor = color;
			if (header)
				currentColor = COL_HEADER;
			if (focusName != "")
				currentColor = COL_LINK;

			bool isBold = bold || header;

			int lt = s.IndexOf("<");
			if (lt < 0)
			{
				AddRunText(runs, s, focusName, isBold, currentFontSize, currentColor);
				return;
			}

			if (lt > 0)
			{
				string textBefore = s.Substring(0, lt);
				AddRunText(runs, textBefore, focusName, isBold, currentFontSize, currentColor);
			}

			string rest = s.Substring(lt, s.Length() - lt);
			int gt = rest.IndexOf(">");
			if (gt < 0)
			{
				AddRunText(runs, rest, focusName, isBold, currentFontSize, currentColor);
				return;
			}

			string tag = rest.Substring(1, gt - 1).Trim();
			string after = rest.Substring(gt + 1, rest.Length() - gt - 1);

			if (tag == "br/" || tag == "br")
			{
				AppendBreak(runs);
				s = after;
			}
			else if (tag == "hr/" || tag == "hr")
			{
				PS_RichTextRun rule = new PS_RichTextRun();
				rule.m_bRule = true;
				runs.Insert(rule);
				s = ConsumeLeadingNewline(after);
			}
			else if (tag == "gap/" || tag == "gap")
			{
				PS_RichTextRun gap = new PS_RichTextRun();
				gap.m_bGap = true;
				runs.Insert(gap);
				s = ConsumeLeadingNewline(after);
			}
			else if (tag == "h")
			{
				EnsureLineStart(runs);
				header = true;
				s = after;
			}
			else if (tag == "/h")
			{
				header = false;
				AppendBreak(runs);
				s = ConsumeLeadingNewline(after);
			}
			else if (tag == "b")
			{
				bold = true;
				s = after;
			}
			else if (tag == "/b")
			{
				bold = false;
				s = after;
			}
			else if (tag.StartsWith("link="))
			{
				focusName = tag.Substring(5, tag.Length() - 5);
				s = after;
			}
			else if (tag == "/link")
			{
				focusName = "";
				s = after;
			}
			else if (tag.StartsWith("color="))
			{
				colorStack.Insert(color);
				color = ParseColorSpec(tag.Substring(6, tag.Length() - 6));
				s = after;
			}
			else if (tag == "/color")
			{
				if (!colorStack.IsEmpty())
				{
					color = colorStack[colorStack.Count() - 1];
					colorStack.Remove(colorStack.Count() - 1);
				}
				else
				{
					color = COL_TEXT;
				}
				s = after;
			}
			else
			{
				s = after;
			}
		}
	}

	protected void AddRunText(notnull array<ref PS_RichTextRun> runs, string text, string focus, bool bold, int fontSize, int color)
	{
		if (text == "")
			return;
		PS_RichTextRun run = new PS_RichTextRun();
		run.m_sText = text;
		run.m_sFocusName = focus;
		run.m_bBold = bold;
		run.m_iFontSize = fontSize;
		run.m_iColor = color;
		runs.Insert(run);
	}

	protected string ConsumeLeadingNewline(string s)
	{
		int len = s.Length();
		int i = 0;
		if (i < len && s.Get(i) == "\r")
			i++;
		if (i < len && s.Get(i) == "\n")
			return s.Substring(i + 1, len - i - 1);
		return s;
	}

	protected void EnsureLineStart(array<ref PS_RichTextRun> runs)
	{
		if (runs.IsEmpty())
			return;
		PS_RichTextRun last = runs[runs.Count() - 1];
		if (last.m_bBreak || last.m_bRule)
			return;
		AppendBreak(runs);
	}

	protected void AppendBreak(array<ref PS_RichTextRun> runs)
	{
		PS_RichTextRun br = new PS_RichTextRun();
		br.m_bBreak = true;
		runs.Insert(br);
	}

	protected void CreateWords(WorkspaceWidget ws, notnull array<ref PS_RichTextRun> runs)
	{
		int pendingSpaces = 0;
		foreach (PS_RichTextRun run : runs)
		{
			if (run.m_bBreak)
			{
				PS_RichTextWord br = new PS_RichTextWord();
				br.m_bBreak = true;
				m_aWords.Insert(br);
				pendingSpaces = 0;
				continue;
			}

			if (run.m_bRule)
			{
				PS_RichTextWord rw = new PS_RichTextWord();
				rw.m_bRule = true;
				rw.m_wText = ws.CreateWidget(WidgetType.ImageWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.INHERIT_CLIPPING, SRGB(COL_DIVIDER), 0, m_wFrame);
				m_aWords.Insert(rw);
				pendingSpaces = 0;
				continue;
			}

			if (run.m_bGap)
			{
				PS_RichTextWord gap = new PS_RichTextWord();
				gap.m_bGap = true;
				m_aWords.Insert(gap);
				pendingSpaces = 0;
				continue;
			}

			bool isLink = run.m_sFocusName != "";
			bool firstWordOfRun = true;

			string txt = run.m_sText;
			int len = txt.Length();
			int i = 0;
			while (i < len)
			{
				while (i < len)
				{
					string c = txt.Get(i);
					if (c == " ")
						pendingSpaces++;
					else if (c == "\n")
					{
						PS_RichTextWord nl = new PS_RichTextWord();
						nl.m_bBreak = true;
						m_aWords.Insert(nl);
						pendingSpaces = 0;
						firstWordOfRun = true;
					}
					else if (c != "\r")
						break;
					i++;
				}
				if (i >= len)
					break;

				int start = i;
				while (i < len && txt.Get(i) != " " && txt.Get(i) != "\n" && txt.Get(i) != "\r")
					i++;
				string word = txt.Substring(start, i - start);

				PS_RichTextWord lw = new PS_RichTextWord();
				lw.m_iLeadingSpaces = pendingSpaces;
				lw.m_bLinkCont = isLink && !firstWordOfRun;
				firstWordOfRun = false;
				pendingSpaces = 0;

				Widget t = ws.CreateWidget(WidgetType.TextWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.INHERIT_CLIPPING, SRGB(run.m_iColor), 0, m_wFrame);
				TextWidget tw = TextWidget.Cast(t);
				if (tw)
				{
					tw.SetFont(FONT);
					tw.SetExactFontSize(run.m_iFontSize);
					tw.SetBold(run.m_bBold);
					tw.SetText(word);
				}
				FrameSlot.SetSizeToContent(t, true);
				lw.m_wText = t;

				if (isLink)
				{
					Widget ul = ws.CreateWidget(WidgetType.ImageWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.IGNORE_CURSOR | WidgetFlags.INHERIT_CLIPPING, SRGB(COL_LINK), 1, m_wFrame);
					lw.m_wUnderline = ul;

					Widget btn = ws.CreateWidget(WidgetType.ButtonWidgetTypeID, WidgetFlags.VISIBLE | WidgetFlags.INHERIT_CLIPPING, Color.FromInt(0x00000000), 2, m_wFrame);
					PS_RichTextLinkHandler h = new PS_RichTextLinkHandler();
					h.Setup(run.m_sFocusName);
					btn.AddHandler(h);
					lw.m_wButton = btn;
				}

				m_aWords.Insert(lw);
			}
		}
	}

	protected bool WordsMeasurable()
	{
		foreach (PS_RichTextWord lw : m_aWords)
		{
			if (lw.m_bBreak || lw.m_bRule || lw.m_bGap || !lw.m_wText)
				continue;
			float sx = 0;
			float sy = 0;
			lw.m_wText.GetScreenSize(sx, sy);
			return sx > 0;
		}
		return true;
	}

	protected void DoLayout()
	{
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (!ws || !m_wFrame)
		{
			s_Pending.RemoveItem(this);
			return;
		}

		if (!WordsMeasurable() && m_iLayoutTries < MAX_LAYOUT_TRIES)
		{
			m_iLayoutTries++;
			GetGame().GetCallqueue().CallLater(DoLayout, 0, false);
			return;
		}

		s_Pending.RemoveItem(this);

		float x = 0;
		float y = 0;
		float curLineH = 0;

		foreach (PS_RichTextWord lw : m_aWords)
		{
			if (lw.m_bBreak)
			{
				y += LineAdvance(curLineH);
				curLineH = 0;
				x = 0;
				continue;
			}

			if (lw.m_bGap)
			{
				y += curLineH + GAP_HEIGHT;
				curLineH = 0;
				x = 0;
				continue;
			}

			if (lw.m_bRule)
			{
				y += curLineH;
				curLineH = 0;
				x = 0;
				if (lw.m_wText)
				{
					FrameSlot.SetAlignment(lw.m_wText, 0, 0);
					FrameSlot.SetPos(lw.m_wText, 0, y + DIVIDER_PAD);
					FrameSlot.SetSize(lw.m_wText, m_fMaxWidth, DIVIDER_THICKNESS);
				}
				y += DIVIDER_ROW_HEIGHT;
				continue;
			}

			if (!lw.m_wText)
				continue;

			float sx = 0;
			float sy = 0;
			lw.m_wText.GetScreenSize(sx, sy);
			if (sx <= 0)
			{
				TextWidget tw = TextWidget.Cast(lw.m_wText);
				if (tw)
					tw.GetTextSize(sx, sy);
			}
			float w = ws.DPIUnscale(sx);
			float h = ws.DPIUnscale(sy);
			if (h <= 0)
				h = FALLBACK_LINE_HEIGHT;

			if (sy > 0 && h <= FONT_SIZE + 6)
				m_fLineHeight = h;

			float adv = lw.m_iLeadingSpaces * SPACE;

			bool wrapped = false;
			if (x > 0 && x + adv + w > m_fMaxWidth)
			{
				y += LineAdvance(curLineH);
				curLineH = 0;
				x = 0;
				wrapped = true;
			}
			else
			{
				x += adv;
			}

			FrameSlot.SetAlignment(lw.m_wText, 0, 0);
			FrameSlot.SetPos(lw.m_wText, x, y);

			if (lw.m_wUnderline)
			{
				float ulX = x;
				float ulW = w;
				if (lw.m_bLinkCont && !wrapped)
				{
					ulX = x - adv;
					ulW = w + adv;
				}
				FrameSlot.SetAlignment(lw.m_wUnderline, 0, 0);
				FrameSlot.SetPos(lw.m_wUnderline, ulX, y + h - UNDERLINE_GAP);
				FrameSlot.SetSize(lw.m_wUnderline, ulW, UNDERLINE_THICKNESS);
			}
			if (lw.m_wButton)
			{
				FrameSlot.SetAlignment(lw.m_wButton, 0, 0);
				FrameSlot.SetPos(lw.m_wButton, x, y);
				FrameSlot.SetSize(lw.m_wButton, w, h);
			}

			if (h > curLineH)
				curLineH = h;
			x += w;
		}

		y += LineAdvance(curLineH);
		FrameSlot.SetSize(m_wFrame, m_fMaxWidth, y);
	}

	protected float LineAdvance(float curLineH)
	{
		if (curLineH > 0)
			return curLineH;
		return m_fLineHeight;
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
}
