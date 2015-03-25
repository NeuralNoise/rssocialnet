#include <iostream>
#include <QTimer>
#include "RsWebUIConfig.h"

RsWebUIConfig::RsWebUIConfig(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);
#ifdef USE_OLD_WITTY_CODE
	 _current_ip_list.clear();
	 _old_port = RSWebUI::port() ;
#endif
	 _ip_list_changed = false ;

	 QObject::connect(ui.IPmask_LE,SIGNAL(textChanged(const QString&)),this,SLOT(on_IPmaskChanged(const QString&))) ;
	 QObject::connect(ui.enableWebUI_CB,SIGNAL(toggled(bool)),this,SLOT(on_enableSwitch(bool))) ;
}

RsWebUIConfig::~RsWebUIConfig() {}

/** Loads the settings for this page */
void RsWebUIConfig::load()
{
    loadSettings();

	 on_IPmaskChanged(ui.IPmask_LE->text()) ;
#ifdef USE_OLD_WITTY_CODE
	 ui.enableWebUI_CB->setChecked(RSWebUI::isRunning()) ;
#endif
}

static QString IPmaskToString(uint32_t ip_mask)
{
	QString out ;

	for(int i=0;i<4;++i)
	{
		if(out.isNull())
			out = QString::number(ip_mask & 0xff) ;
		else
			out = QString::number(ip_mask & 0xff) + "." + out;
		ip_mask >>= 8 ;
	}
	return out ;
}
static uint32_t stringToIPmask(const QString IP_string,bool& ok)
{
	uint32_t res = 0 ;
	ok = false ;

	QStringList lst = IP_string.split('.') ;

	if(lst.size() != 4)
		return 0;

	for(QStringList::const_iterator it(lst.begin());it!=lst.end();++it)
	{
		res <<= 8 ;
		std::cerr << "Parsing string " << (*it).toStdString() << std::endl;
		bool b = false ;
		int a = (*it).toInt(&b) ;

		std::cerr << "got a=" << a << std::endl;
		if(!b)
			return 0 ;

		if(a < 0 || a > 255)
			return 0;

		res += a ;

		std::cerr << "now res=" << res << std::endl;
	}

	ok = true ;
	std::cerr << "returning " << res << std::endl;
	return res ;
}

void RsWebUIConfig::loadSettings() 
{
#ifdef USE_OLD_WITTY_CODE
	ui.port_SB->setValue( RSWebUI::port() ) ;
	QString str ;
	for(std::vector<RSWebUI::IPRange>::const_iterator it(RSWebUI::ipMask().begin());it!=RSWebUI::ipMask().end();++it)
		str += QString::fromStdString((*it).toStdString()) ;

	ui.IPmask_LE->setText( str ) ;
#endif
}

bool RsWebUIConfig::save(QString &/*errmsg*/) 
{
	std::cerr << "Saving: checking params..." << std::endl;

	if(_old_port != ui.port_SB->value() || _ip_list_changed)
	{
		//std::cerr << "RsWebUIConfig::save() setting new port to " << ui.port_SB->value() << ", and mask = " << _current_ip_list << std::endl;
#ifdef USE_OLD_WITTY_CODE
		RSWebUI::setPort(ui.port_SB->value()) ;
		RSWebUI::setIPMask(_current_ip_list) ;

		RSWebUI::restart() ;
#endif

		std::cerr << "Restarted Web UI." << std::endl;
	}
	else
		std::cerr << "No changes." << std::endl;

	return true;
}

void RsWebUIConfig::on_IPmaskChanged(const QString& IPmask)
{
	bool b =true;
#ifdef USE_OLD_WITTY_CODE
	std::cerr << "Checking IP mask..." ;
	std::vector<RSWebUI::IPRange> iprlst ;

	QStringList lst = IPmask.split(';') ;

	for(QStringList::const_iterator it(lst.begin());it!=lst.end();++it)
	{
		bool c ;
		RSWebUI::IPRange rng = RSWebUI::IPRange::make_range( (*it).toStdString(),c) ;

		if(c)
			iprlst.push_back(rng) ;
		else
		{
			b = false ;
			break ;
		}
	}
#endif

	QColor color ;

	if(b)
	{
#ifdef USE_OLD_WITTY_CODE
		_old_ip_list = _current_ip_list ;
		_current_ip_list = iprlst ;
#endif
		_ip_list_changed = true ;

		color = QApplication::palette().color(QPalette::Active,QPalette::Base) ;
		std::cerr << " - ok. New mask = " ;
#ifdef USE_OLD_WITTY_CODE
		for(std::vector<RSWebUI::IPRange>::const_iterator it(iprlst.begin());it!=iprlst.end();++it)
			std::cerr << (*it).toStdString() << " - " ;
		std::cerr << std::endl;
#endif
	}
	else
	{
		std::cerr << " - bad" << std::endl;
		color = QApplication::palette().color(QPalette::Disabled,QPalette::Base) ;
	}

	QPalette palette = ui.IPmask_LE->palette();
	palette.setColor(ui.IPmask_LE->backgroundRole(), color);
	ui.IPmask_LE->setPalette(palette);
}
void RsWebUIConfig::on_enableSwitch(bool b)
{
	ui.params_GB->setEnabled(b) ;

	if(!b)
	{
		std::cerr << "Stopping server." << std::endl;
#ifdef USE_OLD_WITTY_CODE
		RSWebUI::stop() ;
#endif
	}
}

QString RsWebUIConfig::helpText() const
{
	int port = ui.port_SB->value() ;

	return QString("<p>This plugin provides a web interface to control Retroshare with your web browser. \
			          Once enabled, you can connect to Retroshare on the same machine, using \
						 <a href=\"http://localhost:%1\">http://localhost:%1</a></p> \
						 <p>The IP mask can be configured to allow only a restricted set of IP ranges. \
						 ").arg(port); 
}


