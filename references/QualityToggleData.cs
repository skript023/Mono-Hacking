using System;
using UnityEngine.UI;

public struct QualityToggleData
{
	public GraphicsSettingBool m_setting;

	public Toggle m_toggle;

	public QualityToggleData(GraphicsSettingBool setting, Toggle toggle)
	{
		m_setting = setting;
		m_toggle = toggle ?? throw new ArgumentNullException("toggle");
	}
}
