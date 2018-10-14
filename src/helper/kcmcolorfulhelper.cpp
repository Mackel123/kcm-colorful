#include "kcmcolorfulhelper.h"
#include "colordata.h"
#include "mmcq.h"
#include <climits>
#include <cmath>
#include <QSettings>
#include <QUuid>
#include <QDebug>
#include <QtGlobal>
#include <QDir>
#include <QThread>

KcmColorfulHelper::KcmColorfulHelper(QString pic, QString colorcode, QString pn, QObject *parent) : QObject(parent)
{
    QTime time = QTime::currentTime();
    qsrand(static_cast<uint>(time.msec()));
    mConfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));

    bool ok = false;
    paletteNum = pn.toInt(&ok);
    if (ok == true) {
        if (paletteNum < 1 && paletteNum > 16) {
            paletteNum = 8;
        }
    } else {
        paletteNum = 8;
    }
    if (pic == QString("IsoaSFlus-NOPIC")) {
        c = new QColor(colorcode);
    } else {
        wallpaperFilePath = pic;
        mmcq = new MMCQ(wallpaperFilePath);
    }


//    genCSName();
//    getPrevCSName();
//    readDefaultCS();
}

KcmColorfulHelper::~KcmColorfulHelper()
{
    delete c;
//    colorExtractProc->terminate();
//    colorExtractProc->waitForFinished(3000);
//    colorExtractProc->deleteLater();
    mConfig->markAsClean();
    tConfig->markAsClean();
}

void KcmColorfulHelper::run()
{
    if (mmcq != nullptr) {
        palette = mmcq->get_palette(paletteNum);
        palette_16 = mmcq->get_palette(16);
        calcColor();
    }
    qDebug().noquote() << QString("Select: %1, %2, %3 \033[48;2;%1;%2;%3m     \033[0m").arg(QString::number(c->red()), QString::number(c->green()), QString::number(c->blue()));
    readTemplateCS();
    changeColorScheme(tConfig);
    save();
}

void KcmColorfulHelper::getPrevCSName()
{
    KSharedConfigPtr cfg = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    KConfigGroup groupOut(cfg, "General");
    prevColorSchemeName = groupOut.readEntry("ColorScheme");

}

void KcmColorfulHelper::readTemplateCS()
{
    QString tName;
    if (isDarkTheme()) {
        tName = "ColorfulDark";
    } else {
        tName = "ColorfulLight";
    }

    QString schemeFile;

    schemeFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "color-schemes/" + tName + ".colors");
    QFile file(schemeFile);

    if (!file.exists()) {
        schemeFile = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                    + "/color-schemes/" + "BreezeIso.colors";
    }
    qDebug() << schemeFile;
    tConfig = KSharedConfig::openConfig(schemeFile);
}

void KcmColorfulHelper::changeColorScheme()
{
    KConfigGroup group(mConfig, "Colors:View");
//    qDebug() << group.readEntry("ForegroundNormal");

    group.writeEntry("ForegroundNormal", addJitter(group.readEntry("ForegroundNormal")));
    group.writeEntry("DecorationHover", *c);


//    KConfigGroup groupWMTheme(config, "WM");
    KConfigGroup groupWMOut(mConfig, "WM");

    QStringList colorItemListWM;
    colorItemListWM << "activeBackground"
                    << "activeForeground"
                    << "inactiveBackground"
                    << "inactiveForeground";
//                    << "activeBlend"
//                    << "inactiveBlend";

    QVector<QColor> defaultWMColors;
    defaultWMColors << *c
                    << QColor(239,240,241)
                    << *c
                    << QColor(189,195,199)
/*                    << QColor(255,255,255)
                    << QColor(75,71,67)*/;

    int i = 0;
    for (const QString &coloritem : colorItemListWM)
    {
            groupWMOut.writeEntry(coloritem, defaultWMColors.value(i));
            ++i;
    }
}

void KcmColorfulHelper::changeColorScheme(KSharedConfigPtr config)
{
//    // store colorscheme name in global settings
//    mConfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
//    KConfigGroup groupOut(mConfig, "General");
//    qDebug() << groupOut.readEntry("ColorScheme");
//    groupOut.writeEntry("ColorScheme", colorSchemeName);

    QStringList colorItemList;
    colorItemList << "BackgroundNormal"
                  << "BackgroundAlternate"
                  << "ForegroundNormal"
                  << "ForegroundInactive"
                  << "ForegroundActive"
                  << "ForegroundLink"
                  << "ForegroundVisited"
                  << "ForegroundNegative"
                  << "ForegroundNeutral"
                  << "ForegroundPositive"
                  << "DecorationFocus"
                  << "DecorationHover";

    QStringList colorSetGroupList;
    colorSetGroupList << "Colors:View"
                      << "Colors:Window"
                      << "Colors:Button"
                      << "Colors:Selection"
                      << "Colors:Tooltip"
                      << "Colors:Complementary";

    QList <KColorScheme> colorSchemes;

    colorSchemes.append(KColorScheme(QPalette::Active, KColorScheme::View, config));
    colorSchemes.append(KColorScheme(QPalette::Active, KColorScheme::Window, config));
    colorSchemes.append(KColorScheme(QPalette::Active, KColorScheme::Button, config));
    colorSchemes.append(KColorScheme(QPalette::Active, KColorScheme::Selection, config));
    colorSchemes.append(KColorScheme(QPalette::Active, KColorScheme::Tooltip, config));
    colorSchemes.append(KColorScheme(QPalette::Active, KColorScheme::Complementary, config));

    for (int i = 0; i < colorSchemes.length(); ++i)
    {
        KConfigGroup group(mConfig, colorSetGroupList.value(i));
        group.writeEntry("BackgroundNormal", addJitter(colorSchemes[i].background(KColorScheme::NormalBackground).color()));
        group.writeEntry("BackgroundAlternate", colorSchemes[i].background(KColorScheme::AlternateBackground).color());
        group.writeEntry("ForegroundNormal", addJitter(colorSchemes[i].foreground(KColorScheme::NormalText).color()));
        group.writeEntry("ForegroundInactive", colorSchemes[i].foreground(KColorScheme::InactiveText).color());
        group.writeEntry("ForegroundActive", colorSchemes[i].foreground(KColorScheme::ActiveText).color());
        group.writeEntry("ForegroundLink", colorSchemes[i].foreground(KColorScheme::LinkText).color());
        group.writeEntry("ForegroundVisited", colorSchemes[i].foreground(KColorScheme::VisitedText).color());
        group.writeEntry("ForegroundNegative", colorSchemes[i].foreground(KColorScheme::NegativeText).color());
        group.writeEntry("ForegroundNeutral", colorSchemes[i].foreground(KColorScheme::NeutralText).color());
        group.writeEntry("ForegroundPositive", colorSchemes[i].foreground(KColorScheme::PositiveText).color());
        group.writeEntry("DecorationFocus", colorSchemes[i].decoration(KColorScheme::FocusColor).color());
        if (i == 0) {
           group.writeEntry("DecorationHover", *c);
        } else {
            group.writeEntry("DecorationHover", colorSchemes[i].decoration(KColorScheme::HoverColor).color());
        }
    }

    KConfigGroup groupWMTheme(config, "WM");
    KConfigGroup groupWMOut(mConfig, "WM");

    QStringList colorItemListWM;
    colorItemListWM << "activeBackground"
                    << "activeForeground"
                    << "inactiveBackground"
                    << "inactiveForeground"
                    << "activeBlend"
                    << "inactiveBlend";

    QVector<QColor> defaultWMColors;
    defaultWMColors << *c
                    << colorSchemes[1].foreground(KColorScheme::NormalText).color()
                    << *c
                    << colorSchemes[1].foreground(KColorScheme::NormalText).color()
                    << QColor(255,255,255)
                    << QColor(75,71,67);

    int i = 0;
    for (const QString &coloritem : colorItemListWM)
    {
        groupWMOut.writeEntry(coloritem, defaultWMColors.value(i));
        ++i;
    }

    QStringList groupNameList;
    groupNameList << "ColorEffects:Inactive" << "ColorEffects:Disabled";

    QStringList effectList;
    effectList << "Enable"
               << "ChangeSelectionColor"
               << "IntensityEffect"
               << "IntensityAmount"
               << "ColorEffect"
               << "ColorAmount"
               << "Color"
               << "ContrastEffect"
               << "ContrastAmount";

    for (const QString &groupName : groupNameList)
    {

        KConfigGroup groupEffectOut(mConfig, groupName);
        KConfigGroup groupEffectTheme(config, groupName);

        for (const QString &effect : effectList) {
            groupEffectOut.writeEntry(effect, groupEffectTheme.readEntry(effect));
        }
    }
}

void KcmColorfulHelper::save()
{
    mConfig->sync();

//    KConfig cfg(QStringLiteral("kcmdisplayrc"), KConfig::NoGlobals);
//    KConfigGroup displayGroup(&cfg, "X11");

//    displayGroup.writeEntry("exportKDEColors", true);

//    cfg.sync();

//    QSettings* settings = new QSettings(QLatin1String("Trolltech"));
//    KSharedConfigPtr kglobalcfg = KSharedConfig::openConfig( "kdeglobals" );
//    KConfigGroup kglobals(kglobalcfg, "KDE");
//    QPalette newPal = KColorScheme::createApplicationPalette(kglobalcfg);
//    applyQtColors(kglobalcfg, *settings, newPal);
//    delete settings;
//    runRdb(KRdbExportQtColors | KRdbExportGtkTheme |  KRdbExportColors);

    QDBusMessage messageA = QDBusMessage::createSignal(QStringLiteral("/KGlobalSettings"), QStringLiteral("org.kde.KGlobalSettings"), QStringLiteral("notifyChange") );
    QList<QVariant> args;
    args.append(0);//previous KGlobalSettings::PaletteChanged. This is now private API in khintsettings
    args.append(0);//unused in palette changed but needed for the DBus signature
    messageA.setArguments(args);
    QDBusConnection::sessionBus().send(messageA);
    QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/KWin"), QStringLiteral("org.kde.KWin"), QStringLiteral("reloadConfig"));
    QDBusConnection::sessionBus().send(message);

    QThread::sleep(1);
    QDBusConnection::sessionBus().send(messageA);
}

void KcmColorfulHelper::genCSName()
{
    colorSchemeName = "" + QUuid::createUuid().toString(QUuid::WithoutBraces).mid(0, 6);
}

void KcmColorfulHelper::saveCSFile()
{

    QString newpath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + "/color-schemes/";
    QProcess::execute(QString("bash -c \"rm '%1Colorful-*'\"").arg(newpath));
    QDir dir;
    dir.mkpath(newpath);
    newpath += colorSchemeName + ".colors";


    KConfig *config = mConfig->copyTo(newpath);
//    mConfig->markAsClean();
//    m_config->reparseConfiguration();
    KConfigGroup groupA(config, "General");
    groupA.writeEntry("Name", colorSchemeName);

    KConfigGroup group(config, "Colors:View");
    group.writeEntry("DecorationHover", *c);


//    KConfigGroup groupWMTheme(config, "WM");
    KConfigGroup groupWMOut(config, "WM");

    QStringList colorItemListWM;
    colorItemListWM << "activeBackground"
                    << "activeForeground"
                    << "inactiveBackground"
                    << "inactiveForeground";
//                    << "activeBlend"
//                    << "inactiveBlend";

    QVector<QColor> defaultWMColors;
    defaultWMColors << *c
                    << QColor(239,240,241)
                    << *c
                    << QColor(189,195,199)
/*                    << QColor(255,255,255)
                    << QColor(75,71,67)*/;

    int i = 0;
    for (const QString &coloritem : colorItemListWM)
    {
            groupWMOut.writeEntry(coloritem, defaultWMColors.value(i));
            ++i;
    }


    // sync it and delete pointer
    config->sync();
//    changeColorScheme(config);
    delete config;
    KSharedConfigPtr rConfig = KSharedConfig::openConfig(newpath);
    changeColorScheme(rConfig);

}

QColor KcmColorfulHelper::addJitter(QColor color)
{
    return QColor(color.red() + (qrand()%5 - 2), color.green() + (qrand()%5 - 2), color.blue() + (qrand()%5 - 2));
}

bool KcmColorfulHelper::isDarkTheme()
{
    if (c->red() < 66 && c->green() < 66 && c->blue() < 80) {
        return true;
    } else {
        return false;
    }
}

void KcmColorfulHelper::calcColor()
{
    double p_min_a = 99999.0;
    double p_min_b = 99999.0;
    double pt = 0;
    double p_tmp = 0;
    int p_average = 0;

    double weight_of_order[8] = {1, 1, 1, 0.95, 0.9, 0.8, 0.7, 0.6};
    QColor color_a;
    QColor color_b;
    QList<QColor>::iterator it;
    for (it = palette.begin(); it != palette.end() && (it - palette.begin()) < 8; ++it) {
        pt = 100;
        p_average = (it->red() + it->green() + it->blue()) / 3;
        p_tmp = 4 * sqrt(abs((it->red() + it->green() + it->blue()) - (170 * 3)));
        if (p_tmp < 40.0) {
            p_tmp = 40.0;
        }
        pt += p_tmp;
        qDebug().noquote() << pt;
        p_tmp = 1.5 * sqrt((pow(it->red() - p_average, 2) + pow(it->green() - p_average, 2) + pow(it->blue() - p_average, 2)) / 3);
        if (p_tmp > 40.0) {
            p_tmp = 40.0;
        }
        pt -= p_tmp;
        qDebug().noquote() << pt;
        p_tmp = ((((it->green() + it->blue()) / 2) > it->red()) ? (((it->green() + it->blue()) / 2 - it->red()) * 0.8 - 1.5 * abs(it->green() - it->blue())) : 0);
        pt += (p_tmp > 0 ? p_tmp : 0);
        qDebug().noquote() << pt;
//        p_tmp += 0.5 * it->green();
        pt = ((pt < 0) ? (weight_of_order[it - palette.begin()]) : (1.0 / weight_of_order[it - palette.begin()])) * pt;
//        qDebug().noquote() << ((p_tmp < 0) ? (weight_of_order[it - palette.begin()]) : (1.0 / weight_of_order[it - palette.begin()]));
        qDebug().noquote() << QString("%1, %2, %3 \033[48;2;%1;%2;%3m     \033[0m weight: %4").arg(QString::number(it->red()), QString::number(it->green()), QString::number(it->blue()), QString::number(pt));
        if (pt < p_min_a) {
            color_a = *it;
            p_min_a = pt;
        }
    }

    qDebug().noquote() << "=============";

    for (it = palette_16.begin(); it != palette_16.end() && (it - palette_16.begin()) < 8; ++it) {
        pt = 100;
        p_average = (it->red() + it->green() + it->blue()) / 3;
        p_tmp = 4 * sqrt(abs((it->red() + it->green() + it->blue()) - (170 * 3)));
        if (p_tmp < 40.0) {
            p_tmp = 40.0;
        }
        pt += p_tmp;
        qDebug().noquote() << pt;
        p_tmp = 1.5 * sqrt((pow(it->red() - p_average, 2) + pow(it->green() - p_average, 2) + pow(it->blue() - p_average, 2)) / 3);
        if (p_tmp > 40.0) {
            p_tmp = 40.0;
        }
        pt -= p_tmp;
        qDebug().noquote() << pt;
        p_tmp = ((((it->green() + it->blue()) / 2) > it->red()) ? (((it->green() + it->blue()) / 2 - it->red()) * 0.8 - 1.5 * abs(it->green() - it->blue())) : 0);
        pt += (p_tmp > 0 ? p_tmp : 0);
        qDebug().noquote() << pt;
//        p_tmp += 0.5 * it->green();
        pt = ((pt < 0) ? (weight_of_order[it - palette_16.begin()]) : (1.0 / weight_of_order[it - palette_16.begin()])) * pt;
//        qDebug().noquote() << ((p_tmp < 0) ? (weight_of_order[it - palette.begin()]) : (1.0 / weight_of_order[it - palette.begin()]));
        qDebug().noquote() << QString("%1, %2, %3 \033[48;2;%1;%2;%3m     \033[0m weight: %4").arg(QString::number(it->red()), QString::number(it->green()), QString::number(it->blue()), QString::number(pt));
        if (pt < p_min_b) {
            color_b = *it;
            p_min_b = pt;
        }
    }

    if (p_min_a < p_min_b) {
        c = new QColor(color_a);
    } else {
        c = new QColor(color_b);
    }


//    int d_min = INT_MAX;
//    int tmp = 0;
//    int index = 0;
//    for (int i = 0; i < 256; i++) {
//        tmp = pow(colordata[i][0] - color.red(), 2) + pow(colordata[i][1] - color.green(), 2) + pow(colordata[i][2] - color.blue(), 2);
//        if (tmp < d_min) {
//            index = i;
//            d_min = tmp;
//        }
//    }
//    c = new QColor(colordata[index][0], colordata[index][1], colordata[index][2]);
//    c = new QColor(color);
}


