modded class SCR_NotificationMessageUIComponent
{
	override void Init(SCR_NotificationData data, SCR_NotificationsLogComponent notificationLog, float fadeDelay)
	{
		// Ping hook
		if (data.GetID() >= 400 && data.GetID() < 500)
		{
			int param1, param2, param3, param4, param5, param6;
			data.GetParams(param1, param2, param3, param4, param5, param6);
			string customText = PS_CustomTextNotificationComponent.GetTextData(param1);
			if (customText != "")
			{
				data.SetMeta(0, data.GetDisplayData());
				string entry1, entry2, entry3, entry4, entry5, entry6;
				data.GetNotificationTextEntries(entry1, entry2, entry3, entry4, entry5, entry6);
				entry1 = entry1 + " \"" + customText + "\"";
				data.SetNotificationTextEntries(entry1);
				fadeDelay += customText.Length() * 500;
			}
		}
		super.Init(data, notificationLog, fadeDelay);
	}
}
