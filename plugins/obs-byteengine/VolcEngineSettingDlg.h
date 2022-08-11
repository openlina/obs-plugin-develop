#ifndef VOLCENGINESETTINGDLG_H
#define VOLCENGINESETTINGDLG_H

#include <QDialog>
#include <QJsonObject>

namespace Ui {
class VolcEngineSettingDlg;
}

class VolcEngineSettingDlg : public QDialog
{
    Q_OBJECT
public:
	enum AudioQuality {
		Audio_Normal,
		Audio_High };
    explicit VolcEngineSettingDlg(QJsonObject conf, QWidget *parent = nullptr);
    ~VolcEngineSettingDlg();

private:
	void LoadConfig();
	void SaveConfig();

	bool checkPara();

private slots:
    void on_btn_ok_clicked();
    void on_btn_cancel_clicked();
    void currentComboTextChanged(const QString &text);

private:
    Ui::VolcEngineSettingDlg *ui;
    QJsonObject conf_;
    int m_video_bitrate;
    int m_video_framerate;
    QSize m_video_resolution;
};

#endif // VOLCENGINESETTINGDLG_H
