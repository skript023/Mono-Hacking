using System;
using TMPro;
using UnityEngine.UI;

public struct QualitySliderData
{
	public GraphicsSettingInt m_setting;

	public Slider m_slider;

	public TMP_Text m_valueText;

	public QualitySliderData(GraphicsSettingInt setting, Slider slider, TMP_Text valueText)
	{
		m_setting = setting;
		m_slider = slider ?? throw new ArgumentNullException("slider");
		m_valueText = valueText ?? throw new ArgumentNullException("valueText");
	}
}
