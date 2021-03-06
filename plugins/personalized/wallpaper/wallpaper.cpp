/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2019 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "wallpaper.h"
#include "ui_wallpaper.h"

#include <QDebug>

Wallpaper::Wallpaper()
{
    ui = new Ui::Wallpaper;
    pluginWidget = new CustomWidget;
    pluginWidget->setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(pluginWidget);

    pluginName = tr("background");
    pluginType = PERSONALIZED;

    //初始化gsettings
    const QByteArray id(BACKGROUND);
    bgsettings = new QGSettings(id);

    //构建xmlhandle对象，本地xml文件不存在则自动构建
    xmlhandleObj = new XmlHandle();

    initData();
    component_init();

    //壁纸的控件listview
//    setlistview();
//    setModeldata();
}

Wallpaper::~Wallpaper()
{
    delete ui;
    delete bgsettings;
    delete xmlhandleObj;
//    QMap<QString, QListWidgetItem *>::Iterator it = delItemsMap.begin();
//    QListWidgetItem * delItem;
//    for (; it != delItemsMap.end(); it++){
//        delItem = (QListWidgetItem *)it.value();
//        delete(delItem);
//    }
    delItemsMap.clear();
}

QString Wallpaper::get_plugin_name(){
    return pluginName;
}

int Wallpaper::get_plugin_type(){
    return pluginType;
}

CustomWidget *Wallpaper::get_plugin_ui(){
    return pluginWidget;
}

void Wallpaper::plugin_delay_control(){
}

void Wallpaper::initData(){
    //解析本地xml填充数据
    localwpconf = QString("%1/%2/%3").arg(QDir::homePath()).arg(".config/ukui").arg("wallpaper.xml");
    wallpaperinfosMap = xmlhandleObj->xmlreader(localwpconf);
}

void Wallpaper::component_init(){
    //背景形式
    QStringList formList;
    formList << tr("picture") << tr("color")/* << tr("slideshow")*/ ;
    ui->formComboBox->addItems(formList);

    init_current_status();


    //        QSize IMAGE_SIZE(160, 120);
    //        ui->listWidget->setIconSize(IMAGE_SIZE);
    ui->listWidget->setResizeMode(QListView::Adjust);
    ui->listWidget->setViewMode(QListView::IconMode);
    ui->listWidget->setMovement(QListView::Static);
    ui->listWidget->setSpacing(10);

    SimpleThread * thread = new SimpleThread(wallpaperinfosMap, nullptr);
    connect(thread, &SimpleThread::widgetItemCreate, this, [=](QPixmap pixmap, QString filename){
        append_item(pixmap, filename);
    }, Qt::QueuedConnection);
    connect(thread, &SimpleThread::finished, this, [=]{
        QString filename = bgsettings->get(FILENAME).toString();
        //当前背景形式是壁纸
        if (ui->formComboBox->currentIndex() == 0){
            if (delItemsMap.contains(filename)){
                QListWidgetItem * currentItem = delItemsMap.find(filename).value();
                QWidget * widget = ui->listWidget->itemWidget(currentItem);
                widget->setStyleSheet("QWidget{border: 5px solid #daebff}");
                //            ui->listWidget->setItemSelected(currentItem, true); //???
                ui->listWidget->blockSignals(true);
                ui->listWidget->setCurrentItem(currentItem);
                ui->listWidget->blockSignals(false);
            }
            //设置当前壁纸放置方式
            if (wallpaperinfosMap.contains(filename)){
                QMap<QString, QString> currentwpMap = (QMap<QString, QString>) wallpaperinfosMap.find(filename).value();
                if (currentwpMap.contains("options")){
                    QString opStr = QString::fromLocal8Bit("%1").arg(currentwpMap.find("options").value());
                    ui->wpoptionsComboBox->setCurrentText(tr("%1").arg(opStr));
                }
            }
        }
    });
    connect(thread, &SimpleThread::finished, thread, &SimpleThread::deleteLater);
    thread->start();

    //壁纸放置方式
    QStringList layoutList;
    layoutList << tr("wallpaper") << tr("centered") << tr("scaled") << tr("stretched") << tr("zoom") << tr("spanned");
    ui->wpoptionsComboBox->addItems(layoutList);


    //纯色
    ui->colorListWidget->setResizeMode(QListView::Adjust);
    ui->colorListWidget->setViewMode(QListView::IconMode);
    ui->colorListWidget->setMovement(QListView::Static);
    ui->colorListWidget->setSpacing(10);

    ui->listWidget->setStyleSheet("QListView::item:selected{border: 5px solid #ac4844}");

    QSize ITEM_SIZE(65, 65);
    //自定义item
    QString colorStr = "#99FF33";
    QWidget * widget = new QWidget();
    widget->setAttribute(Qt::WA_DeleteOnClose);
    widget->setStyleSheet(QString("background-color: %1").arg(colorStr));

    QListWidgetItem * item = new QListWidgetItem(ui->colorListWidget);
    item->setSizeHint(ITEM_SIZE);
    item->setData(Qt::UserRole, colorStr);
    ui->colorListWidget->setItemWidget(item, widget);

    QString colorStr2 = "#FFFF00";
    QLabel * widget2 = new QLabel();
    widget2->setAttribute(Qt::WA_DeleteOnClose);
    widget2->setStyleSheet(QString("background-color: %1").arg(colorStr2));

    QListWidgetItem * item2 = new QListWidgetItem(ui->colorListWidget);
    item2->setData(Qt::UserRole, colorStr2);
    item2->setSizeHint(ITEM_SIZE);
    ui->colorListWidget->setItemWidget(item2, widget2);

    ui->colorListWidget->setCurrentItem(item);
    widget->setStyleSheet(QString("background-color: %1; border: 5px solid #ac4844").arg(item->data(Qt::UserRole).toString()));

    connect(ui->colorListWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(colorwp_item_clicked(QListWidgetItem*,QListWidgetItem*)));


    connect(ui->listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(wallpaper_item_clicked(QListWidgetItem*,QListWidgetItem*)));
    connect(ui->formComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(form_combobox_changed(int)));
    connect(ui->wpoptionsComboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(options_combobox_changed(QString)));
    connect(ui->resetBtn, SIGNAL(clicked(bool)), this, SLOT(reset_default_wallpaper()));

    connect(ui->addPushBtn, SIGNAL(clicked(bool)), this, SLOT(add_custom_wallpaper()));
    connect(ui->delPushBtn, SIGNAL(clicked(bool)), this, SLOT(del_wallpaper()));

}

void Wallpaper::colorwp_item_clicked(QListWidgetItem * current, QListWidgetItem * previous){
    QWidget * preWidget = ui->colorListWidget->itemWidget(previous);
    preWidget->setStyleSheet(QString("background-color: %1; border: none").arg(previous->data(Qt::UserRole).toString()));

    QWidget * curWidget = ui->colorListWidget->itemWidget(current);
    curWidget->setStyleSheet(QString("background-color: %1; border: 5px solid #ac4844").arg(current->data(Qt::UserRole).toString()));
}

void Wallpaper::append_item(QPixmap pixmap, QString filename){
    QSize ITEM_SIZE(165, 125);
    //自定义item
    QWidget * widget = new QWidget();
    widget->setAttribute(Qt::WA_DeleteOnClose);
    QHBoxLayout * mainLayout = new QHBoxLayout(widget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QLabel * wpLable = new QLabel(widget);
    wpLable->setPixmap(pixmap);

    mainLayout->addWidget(wpLable);

    widget->setLayout(mainLayout);

    QListWidgetItem * item = new QListWidgetItem(ui->listWidget);
    item->setSizeHint(ITEM_SIZE);
    item->setData(Qt::UserRole, filename);
    ui->listWidget->setItemWidget(item, widget);

    delItemsMap.insert(filename, item);
}

void Wallpaper::init_current_status(){
    QString filename = bgsettings->get(FILENAME).toString();

    //设置当前背景形式
    if (filename == ""){
        ui->formComboBox->setCurrentIndex(1);
        ui->substackedWidget->setCurrentIndex(1);
        ui->addPushBtn->hide();
        ui->delPushBtn->hide();
    }
    else if (filename.endsWith("xml")){
        ui->formComboBox->setCurrentIndex(2);
        ui->substackedWidget->setCurrentIndex(2);
        ui->addPushBtn->hide();
        ui->delPushBtn->hide();
    }
    else{
        ui->formComboBox->setCurrentIndex(0);
        ui->substackedWidget->setCurrentIndex(0);
        ui->addPushBtn->show();
        ui->delPushBtn->show();
    }
}

void Wallpaper::wallpaper_item_clicked(QListWidgetItem * current, QListWidgetItem *previous){
    if (previous != nullptr){
        QWidget * previousWidget = ui->listWidget->itemWidget(previous);
        previousWidget->setStyleSheet("QWidget{border: none}");
    }

    QWidget * currentWidget = ui->listWidget->itemWidget(current);
    currentWidget->setStyleSheet("QWidget{border: 5px solid #daebff}");

    QString filename = current->data(Qt::UserRole).toString();
    bgsettings->set(FILENAME, QVariant(filename));
    init_current_status();
}

void Wallpaper::form_combobox_changed(int index){
    ui->substackedWidget->setCurrentIndex(index);
}

void Wallpaper::options_combobox_changed(QString op){
    //获取当前选中的壁纸
    QListWidgetItem * currentitem = ui->listWidget->currentItem();
    QString filename = currentitem->data(Qt::UserRole).toString();
    qDebug() << filename << "----" << op;

    //更新xml数据
    if (wallpaperinfosMap.contains(filename)){
        wallpaperinfosMap[filename]["options"] = op;
    }

    //将改动保存至文件
    xmlhandleObj->xmlwriter(localwpconf, wallpaperinfosMap);
}

void Wallpaper::setlistview(){
    //初始化listview
//    ui->listView->setFocusPolicy(Qt::NoFocus);
//    ui->listView->setAutoFillBackground(true);
//    ui->listView->setIconSize(QSize(160, 100));
//    ui->listView->setResizeMode(QListView::Adjust);
//    ui->listView->setModel(&wpListModel);
//    ui->listView->setViewMode(QListView::IconMode);
//    ui->listView->setSpacing(5);
}

void Wallpaper::setModeldata(){
    QMap<QString, QMap<QString, QString> >::iterator iters = wallpaperinfosMap.begin();
    for (int row = 0; iters != wallpaperinfosMap.end(); iters++, row++){
        if (QString(iters.key()) == "head") //跳过xml的头部信息
            continue;
        QMap<QString, QString> wpMap = (QMap<QString, QString>)iters.value();
        QString delstatus = QString(wpMap.find("deleted").value());
        if (delstatus == "true") //跳过被删除的壁纸
            continue;

        QString filename = QString(iters.key());
        QPixmap pixmap(filename);

        wpListModel.insertRows(row, 1, QModelIndex());
        QModelIndex wpindex = wpListModel.index(row, 0, QModelIndex());
        wpListModel.setData(wpindex, QIcon(pixmap.scaled(QSize(160,100))), Qt::DecorationRole);
        wpListModel.setData(wpindex, QString("%1\nfolder: %2\n").arg(wpMap.find("name").value()).arg(filename), Qt::ToolTipRole);
    }
}

void Wallpaper::reset_default_wallpaper(){
    GSettings * wpgsettings;
    wpgsettings = g_settings_new(BACKGROUND);
    GVariant * variant = g_settings_get_default_value(wpgsettings, FILENAME);
    gsize size = g_variant_get_size(variant);
    const char * dwp = g_variant_get_string(variant, &size);
    g_object_unref(wpgsettings);

    bgsettings->set(FILENAME, QVariant(QString(dwp)));
    init_current_status();
}

void Wallpaper::add_custom_wallpaper(){
    QString filters = "Wallpaper files(*.png *.jpg)";
    QFileDialog fd;
    fd.setDirectory(QString(const_cast<char *>(g_get_user_special_dir(G_USER_DIRECTORY_PICTURES))));
    fd.setAcceptMode(QFileDialog::AcceptOpen);
    fd.setViewMode(QFileDialog::List);
    fd.setNameFilter(filters);
    fd.setFileMode(QFileDialog::ExistingFile);
    fd.setWindowTitle(tr("selsect custom wallpaper file"));
    fd.setLabelText(QFileDialog::Accept, tr("Select"));
    fd.setLabelText(QFileDialog::LookIn, tr("Position: "));
    fd.setLabelText(QFileDialog::FileName, tr("FileName: "));
    fd.setLabelText(QFileDialog::FileType, tr("FileType: "));
    fd.setLabelText(QFileDialog::Reject, tr("Cancel"));

    if (fd.exec() != QDialog::Accepted)
        return;

    QString selectedfile;
    selectedfile = fd.selectedFiles().first();

    QSize IMAGE_SIZE(160, 120);
    QPixmap pixmap = QPixmap(selectedfile).scaled(IMAGE_SIZE);
    append_item(pixmap, selectedfile);

    if (wallpaperinfosMap.contains(selectedfile)){
        wallpaperinfosMap[selectedfile]["deleted"] = "false";
    }
    else{
        QMap<QString, QString> tmpinfo;
        tmpinfo.insert("artist", "(none)");
        tmpinfo.insert("deleted", "false");
        tmpinfo.insert("filename", selectedfile);
        tmpinfo.insert("name", selectedfile.split("/").last());
        tmpinfo.insert("options", "zoom");
        tmpinfo.insert("pcolor", "#000000");
        tmpinfo.insert("scolor", "#000000");
        tmpinfo.insert("shade_type", "solid");
        wallpaperinfosMap.insert(selectedfile, tmpinfo);

    }
    xmlhandleObj->xmlwriter(localwpconf, wallpaperinfosMap);

    if (delItemsMap.contains(selectedfile)){
        ui->listWidget->setCurrentItem(delItemsMap.find(selectedfile).value());
    }

}

void Wallpaper::del_wallpaper(){
    //获取当前选中的壁纸
    QListWidgetItem * currentitem = ui->listWidget->currentItem();
    QString filename = currentitem->data(Qt::UserRole).toString();

    //更新xml数据
    if (wallpaperinfosMap.contains(filename)){
        wallpaperinfosMap[filename]["deleted"] = "true";

        int row = ui->listWidget->row(currentitem);

        int nextrow = ui->listWidget->count() - 1 - row ? row + 1 : row - 1;

        ui->listWidget->setCurrentItem(ui->listWidget->item(nextrow));

        ui->listWidget->takeItem(row);

    }

//    将改动保存至文件
    xmlhandleObj->xmlwriter(localwpconf, wallpaperinfosMap);
}
