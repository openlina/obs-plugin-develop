#include "VolcEngineSettingDlg.h"
#include "ui_VolcEngineSettingDlg.h"

#include <QMainWindow>
#include <QTextCodec>

#include <QStringList>
#include <QMessageBox>

#include <obs-frontend-api.h>
#include "util/config-file.h"

#include "data.h"
#include "ByteEngineWarp.h"

ByteEngineWarp *g_warp = nullptr;

VolcEngineSettingDlg::VolcEngineSettingDlg(QJsonObject conf, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::VolcEngineSettingDlg)
	, m_video_bitrate(1500)
	, m_video_framerate(15)
	, m_video_resolution(QSize(1920, 1080))
{
    ui->setupUi(this);

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    connect(ui->comboBox_audio_quality, &QComboBox::currentTextChanged, this,
	    &VolcEngineSettingDlg::currentComboTextChanged);
    this->setWindowTitle("Volcengine Settings");

    QStringList list = QStringList{QString::fromLocal8Bit(QByteArray::fromStdString("标准")),
	    QString::fromLocal8Bit(QByteArray::fromStdString ("高清"))};
    ui->comboBox_audio_quality->addItems(list);
    ui->comboBox_audio_quality->setCurrentIndex(0);

    list = QStringList{"1920x1080", "1440x810", "1152x648", "1096x616", "1080x720", "960x540", "852x480", "768x432", "696x392","640x480"};
    ui->comboBox_resolution->addItems(list);
    ui->comboBox_resolution->setCurrentIndex(0);

    ui->lineEdit_appid->setText("");
    ui->lineEdit_roomid->setText("");
    ui->lineEdit_userid->setText("");

    //单流转推功能未上线
    ui->checkBox_publish->setEnabled(false);
    ui->lineEdit_rtmpurl->setEnabled(false);

    QRegularExpression rx("\\d{1,5}x\\d{1,5}");
    QValidator *validator = new QRegularExpressionValidator(rx, this);
    ui->comboBox_resolution->lineEdit()->setValidator(validator);

    LoadConfig();

    
}

VolcEngineSettingDlg::~VolcEngineSettingDlg()
{
    delete ui;
}

void VolcEngineSettingDlg::LoadConfig() {

	config_t *global_config = obs_frontend_get_profile_config();
	if (config_has_user_value(global_config, CONFIG_SETTINGS,
				  CONFIG_RTC_RTMPURL)) {

		std::string url = config_get_string(global_config, CONFIG_SETTINGS,
					       CONFIG_RTC_RTMPURL);
		ui->lineEdit_rtmpurl->setText(
			QString::fromLocal8Bit(url.c_str()));
	}

	if (config_has_user_value(global_config, CONFIG_SETTINGS,
				  CONFIG_VIDEO_BITRATE)) {

		int v_bitrate = config_get_int(global_config, CONFIG_SETTINGS,
					       CONFIG_VIDEO_BITRATE);
		ui->spinBox_video_bitrate->setValue(v_bitrate);
	}
	if (config_has_user_value(global_config, CONFIG_SETTINGS,
				  CONFIG_VIDEO_FRAMERATE)) {
		int v_framerate = config_get_int(global_config, CONFIG_SETTINGS,
						 CONFIG_VIDEO_FRAMERATE);
		ui->spinBox_video_framerate->setValue(v_framerate);
	}

	if (config_has_user_value(global_config, CONFIG_SETTINGS,
				  CONFIG_VIDEO_RESOLUTION)) {
		std::string str_resolution =
			config_get_string(global_config, CONFIG_SETTINGS,
					  CONFIG_VIDEO_RESOLUTION);
		ui->comboBox_resolution->setCurrentText(str_resolution.c_str());
	}

	if (config_has_user_value(global_config, CONFIG_SETTINGS,
				  CONFIG_AUDIO_QUALITY)) {
		AudioQuality a_quality = static_cast<AudioQuality>(config_get_int(global_config,
			CONFIG_SETTINGS, CONFIG_AUDIO_QUALITY));
		ui->comboBox_audio_quality->setCurrentIndex(a_quality);
	}

	if (config_has_user_value(global_config, CONFIG_SETTINGS,
				  CONFIG_RTC_APPID)) {
		std::string str_appid = config_get_string(
			global_config, CONFIG_SETTINGS,
				       CONFIG_RTC_APPID);
		ui->lineEdit_appid->setText(str_appid.c_str());
	}

	if (config_has_user_value(global_config, CONFIG_SETTINGS,
				  CONFIG_RTC_ROOMID)) {
		std::string str_roomid = config_get_string(
			global_config, CONFIG_SETTINGS, CONFIG_RTC_ROOMID);
		ui->lineEdit_roomid->setText(str_roomid.c_str());
	}

	if (config_has_user_value(global_config, CONFIG_SETTINGS,
				  CONFIG_RTC_USERID)) {
		std::string str_userid = config_get_string(
			global_config, CONFIG_SETTINGS, CONFIG_RTC_USERID);
		ui->lineEdit_userid->setText(str_userid.c_str());
	}
}

void VolcEngineSettingDlg::SaveConfig() {

	config_t *global_config = obs_frontend_get_profile_config();

	config_set_int(global_config, CONFIG_SETTINGS, CONFIG_VIDEO_BITRATE,
		       ui->spinBox_video_bitrate->value());
	config_set_int(global_config, CONFIG_SETTINGS, CONFIG_VIDEO_FRAMERATE,
		       ui->spinBox_video_framerate->value());

	std::string str = ui->comboBox_resolution->currentText().toLocal8Bit();
	config_set_string(global_config, CONFIG_SETTINGS, CONFIG_VIDEO_RESOLUTION, str.c_str());
	config_set_int(global_config, CONFIG_SETTINGS, CONFIG_AUDIO_QUALITY,
		       ui->comboBox_audio_quality->currentIndex());

	str = ui->lineEdit_appid->text().toLocal8Bit();
	config_set_string(global_config, CONFIG_SETTINGS, CONFIG_RTC_APPID, str.c_str());
	str = ui->lineEdit_roomid->text().toLocal8Bit();
	config_set_string(global_config, CONFIG_SETTINGS, CONFIG_RTC_ROOMID, str.c_str());
	str = ui->lineEdit_userid->text().toLocal8Bit();
	config_set_string(global_config, CONFIG_SETTINGS, CONFIG_RTC_USERID, str.c_str());
	
	str = ui->lineEdit_rtmpurl->text().toLocal8Bit();
	config_set_string(global_config, CONFIG_SETTINGS, CONFIG_RTC_RTMPURL,
			  str.c_str());


	config_save_safe(global_config, "tmp", nullptr);
}

bool VolcEngineSettingDlg::checkPara()
{
	QString str = ui->comboBox_resolution->currentText();
	int w = 0, h = 0;
	int index_x = str.indexOf("x", Qt::CaseInsensitive);
	w = str.left(index_x).toInt();
	h = str.right(str.length() - index_x - 1).toInt();
	if (str.isEmpty() || w <= 0 || w >= 10000 || h <=0 || h>= 10000) {
		QMessageBox box;
		box.setText("video resolution error");
		box.exec();
		return false;
	}


	if (ui->lineEdit_appid->text().isEmpty() ||
	    ui->lineEdit_roomid->text().isEmpty() ||
	    ui->lineEdit_userid->text().isEmpty()) {
		QMessageBox box;
		box.setText("please input appid/roomid/userid error");
		box.exec();
		return false;
	}
	return true;
}


void VolcEngineSettingDlg::on_btn_ok_clicked() {

	SaveConfig();
	this->accept();
	this->deleteLater();
}

void VolcEngineSettingDlg::on_btn_cancel_clicked()
{
	this->accept();
	this->deleteLater();
}

void VolcEngineSettingDlg::currentComboTextChanged(const QString &text) {

	if (text == QString::fromLocal8Bit(QByteArray::fromStdString("标准"))) {
		ui->stackedWidget->setCurrentWidget(ui->page);
	} else {
		ui->stackedWidget->setCurrentWidget(ui->page_2);
	}
}

 
void *cb(void *mainwin)
{
	auto setting = new VolcEngineSettingDlg(QJsonObject(), (QMainWindow *)mainwin);
	setting->exec();	

	if (g_warp) {
		g_warp->resetEncoderConfig();
	}

	return (void *)setting;
}

bool settings_load()
{
	auto mainwin = (QMainWindow *)obs_frontend_get_main_window();

	if (mainwin == nullptr)
		return false;
	obs_frontend_add_tools_menu_item("VolcEngine Settings",
					 (obs_frontend_cb)cb, mainwin);

	return true;
}

extern "C" void module_load_settings()
{
	settings_load();
}
