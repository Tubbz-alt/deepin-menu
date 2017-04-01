/**
 * Copyright (C) 2015 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include <QRect>
#include <QPainter>
#include <QBrush>
#include <QAction>
#include <QtGlobal>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QIcon>
#include <QColor>
#include <QJsonObject>
#include <QJsonArray>
#include <QPoint>
#include <QStaticText>
#include <QDebug>

#include "utils.h"
#include "dmenucontent.h"
#include "ddockmenu.h"

#define MENU_ITEM_MAX_WIDTH 300
#define MENU_ITEM_HEIGHT 24
#define MENU_ITEM_FONT_SIZE 13
#define SEPARATOR_HEIGHT 6

static const int LeftRightPadding = 20;
static const int TopBottomPadding = 4;

DMenuContent::DMenuContent(DDockMenu *parent) :
    QWidget(parent),
    _currentIndex(-1)
{
    this->setMouseTracking(true);
}

int DMenuContent::currentIndex()
{
    return _currentIndex;
}

void DMenuContent::setCurrentIndex(int index)
{
    if (index < 0 || _currentIndex == index) return;

    _currentIndex = index;
    this->update();

    if (index < 0 || index >= this->actions().count()) return;

    DDockMenu *parent = qobject_cast<DDockMenu*>(this->parent());
    Q_ASSERT(parent);

    QAction *action = this->actions().at(index);
    QRect actionRect = this->getRectOfActionAtIndex(index);
    QPoint point = this->mapToGlobal(QPoint(this->width(), actionRect.y()));
    QString itemId = action->property("itemId").toString();

    QString prop("%1Active");
    QVariant activeCache = parent->getRootMenu()->property(prop.arg(itemId).toLatin1());
    bool active = activeCache.isNull() ? action->isEnabled() : activeCache.toBool();

    QJsonObject subMenuJsonObj = active ?
                this->actions().at(index)->property("itemSubMenu").value<QJsonObject>()
              : QJsonObject();
    parent->showSubMenu(point.x(), point.y(), subMenuJsonObj);
}

int DMenuContent::contentWidth()
{
    int result = 0;

    QFont font;
    font.setPixelSize(MENU_ITEM_FONT_SIZE);
    QFontMetrics metrics(font);

    foreach (QAction *action, this->actions()) {
        result = qMax(result, metrics.width(action->text()));
    }

    return qMin(MENU_ITEM_MAX_WIDTH, result + 10 + LeftRightPadding*2);
}

int DMenuContent::contentHeight()
{
    int result = 0;

    foreach (QAction *action, this->actions()) {
        if (action->text().isEmpty()) {
            result += SEPARATOR_HEIGHT;
        } else {
            result += MENU_ITEM_HEIGHT;
        }
    }

    return result + TopBottomPadding * 2;
}

void DMenuContent::doCurrentAction()
{
    if (_currentIndex < 0 || _currentIndex >= this->actions().count()) return;

    DDockMenu *parent = qobject_cast<DDockMenu*>(this->parent());
    Q_ASSERT(parent);

    QAction *currentAction = this->actions().at(_currentIndex);
    QString itemId = currentAction->property("itemId").toString();
    QString prop("%1Checked");
    QVariant checkedCache = parent->getRootMenu()->property(prop.arg(itemId).toLatin1());
    bool checked = checkedCache.isNull() ? currentAction->isChecked() : checkedCache.toBool();
    bool currentActionHasSubMenu = currentAction->property("itemSubMenu").value<QJsonObject>()["items"].toArray().count() != 0;
    prop = QString("%1Activity");
    QVariant activeCache = parent->getRootMenu()->property(prop.arg(itemId).toLatin1());
    bool active = activeCache.isNull() ? currentAction->isEnabled() : activeCache.toBool();

    if (!active || currentAction->text().isEmpty() || currentActionHasSubMenu) return;

    if (currentAction->isCheckable()) {
        if (checked) {
            this->doUnCheck(_currentIndex);
        } else {
            this->doCheck(_currentIndex);
        }
    } else {
        parent->releaseFocus();
        this->sendItemClickedSignal(currentAction->property("itemId").toString(), false);
    }
    parent->destroyAll();
}

void DMenuContent::grabFocus()
{
    QTimer::singleShot(500, this, [this] {
        grabKeyboard();
        grabMouse();
    });
}

// override methods
void DMenuContent::paintEvent(QPaintEvent *)
{
    DDockMenu *parent = qobject_cast<DDockMenu*>(this->parent());
    QFont font;
    font.setPixelSize(MENU_ITEM_FONT_SIZE);
    QPainter painter(this);
    painter.setFont(font);

    for(int i = 0; i < this->actions().count(); i++) {
        QAction *action = this->actions().at(i);
        QRect actionRect = this->getRectOfActionAtIndex(i);
        QString itemId = action->property("itemId").toString();

        QString prop("%1Activity");
        QVariant activeCache = parent->getRootMenu()->property(prop.arg(itemId).toLatin1());
        bool active = activeCache.isNull() ? action->isEnabled() : activeCache.toBool();
        ItemStyle itemStyle = active ? i == _currentIndex ? parent->hoverStyle : parent->normalStyle : parent->inactiveStyle;

        // indicates that this item is a separator
        if (action->text().isEmpty()) {
            int topLineX1 = actionRect.x() + 4;
            int topLineY1 = actionRect.y() + (actionRect.height() - 2) / 2;
            int topLineX2 = actionRect.x() + actionRect.width() - 4;
            int topLineY2 = actionRect.y() + (actionRect.height() - 2) / 2;
            int bottomLineX1 = topLineX1;
            int bottomLineY1 = topLineY1 + 1;
            int bottomLineX2 = topLineX2;
            int bottomLineY2 = topLineY1 + 1;
            painter.setPen(QPen(QColor::fromRgbF(0, 0, 0, 0.1)));
            painter.drawLine(topLineX1, topLineY1, topLineX2, topLineY2);
            painter.setPen(QPen(QColor::fromRgbF(1, 1, 1, 0.1)));
            painter.drawLine(bottomLineX1, bottomLineY1, bottomLineX2, bottomLineY2);
        } else {
            painter.fillRect(actionRect, QBrush(itemStyle.itemBackgroundColor));
            painter.setPen(QPen(itemStyle.itemTextColor));

            // draw text
            QString prop("%1Text");
            QVariant textCache = parent->getRootMenu()->property(prop.arg(itemId).toLatin1());
            QString text = textCache.isNull() ? action->text() : textCache.toString();
            QString elidedText = elideText(text, actionRect.width() - LeftRightPadding * 2);

            QTextOption option;
            option.setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

            QRect textRect(actionRect);
            textRect.adjust(LeftRightPadding, 0, -LeftRightPadding, 0);
            painter.drawText(textRect, elidedText, option);
        }
    }

    painter.end();
}

void DMenuContent::mouseMoveEvent(QMouseEvent *event)
{
    int index = itemIndexUnderEvent(event);
    setCurrentIndex(index);
}

void DMenuContent::mouseReleaseEvent(QMouseEvent *event)
{
    DDockMenu *parent = qobject_cast<DDockMenu*>(this->parent());
    if (parent) {
        DDockMenu *menu = parent->menuUnderPoint(event->globalPos());
        if (menu) {
            doCurrentAction();
        } else {
            parent->destroyAll();
        }
    }
}

void DMenuContent::keyPressEvent(QKeyEvent *event)
{
    DDockMenu *parent = qobject_cast<DDockMenu*>(this->parent());
    Q_ASSERT(parent);

    switch (event->key()) {
    case Qt::Key_Escape:
        parent->destroyAll();
        break;
    case Qt::Key_Return:
        this->doCurrentAction();
        break;
    case Qt::Key_Up:
        this->selectPrevious();
        break;
    case Qt::Key_Down:
        this->selectNext();
        break;
    case Qt::Key_Right:
        break;
    case Qt::Key_Left:
        if (parent->parent()) {
            DDockMenu *p_parent = qobject_cast<DDockMenu*>(parent->parent());
            Q_ASSERT(p_parent);

            p_parent->grabFocus();
        }
        break;
    }
}

// private methods
QRect DMenuContent::getRectOfActionAtIndex(int index)
{
    int previousHeight = TopBottomPadding;
    int itemHeight = this->actions().at(index)->text().isEmpty() ? SEPARATOR_HEIGHT : MENU_ITEM_HEIGHT;

    for (int i = 0; i < index; i++) {
        QAction *action = this->actions().at(i);
        if (action->text().isEmpty()) {
            previousHeight += SEPARATOR_HEIGHT;
        } else {
            previousHeight += MENU_ITEM_HEIGHT;
        }
    }

    return QRect(0, previousHeight, this->width(), itemHeight);
}

void DMenuContent::clearActions()
{
    foreach (QAction *action, this->actions()) {
        this->removeAction(action);
    }
}

int DMenuContent::getNextItemsHasShortcut(int startPos, QString keyText) {
    if (keyText.isEmpty()) return -1;

    DDockMenu *parent = qobject_cast<DDockMenu*>(this->parent());
    Q_ASSERT(parent);

    for (int i = qMax(startPos, 0); i < this->actions().count(); i++) {
        QString itemId(this->actions().at(i)->property("itemId").toString());
        QAction *action = this->actions().at(i);

        QString prop("%1Active");
        QVariant activeCache = parent->getRootMenu()->property(prop.arg(itemId).toLatin1());
        bool active = activeCache.isNull() ? action->isEnabled() : activeCache.toBool();

        // a trick here, using currentIndex as the cursor.
        if (active && keyText == this->actions().at(i)->property("itemNavKey").toString().toLower()){
            return i;
        }
    }

    // we do the check another time to support wrapping.
    for (int i = 0; i < this->actions().count(); i++) {
        QString itemId(this->actions().at(i)->property("itemId").toString());
        QAction *action = this->actions().at(i);

        QString prop("%1Active");
        QVariant activeCache = parent->getRootMenu()->property(prop.arg(itemId).toLatin1());
        bool active = activeCache.isNull() ? action->isEnabled() : activeCache.toBool();

        if (active && keyText == this->actions().at(i)->property("itemNavKey").toString().toLower()) {
            return i;
        }
    }
    return -1;
}

void DMenuContent::selectPrevious()
{
    DDockMenu *parent = qobject_cast<DDockMenu*>(this->parent());
    Q_ASSERT(parent);

    for (int i = currentIndex() - 1; i >= 0; i--) {
        QAction * action = actions().at(i);

        QString itemId(action->property("itemId").toString());

        QString prop("%1Active");
        QVariant activeCache = parent->getRootMenu()->property(prop.arg(itemId).toLatin1());
        bool active = activeCache.isNull() ? action->isEnabled() : activeCache.toBool();

        if (active && !action->text().isEmpty()) {
            setCurrentIndex(i);
            break;
        }
    }
}

void DMenuContent::selectNext()
{
    DDockMenu *parent = qobject_cast<DDockMenu*>(this->parent());
    Q_ASSERT(parent);

    for (int i = currentIndex() + 1; i < actions().count(); i++) {
        QAction * action = actions().at(i);

        QString itemId(action->property("itemId").toString());

        QString prop("%1Active");
        QVariant activeCache = parent->getRootMenu()->property(prop.arg(itemId).toLatin1());
        bool active = activeCache.isNull() ? action->isEnabled() : activeCache.toBool();

        if (active && !action->text().isEmpty()) {
            setCurrentIndex(i);
            break;
        }
    }
}

void DMenuContent::doCheck(int index) {
    DDockMenu *parent = qobject_cast<DDockMenu*>(this->parent());
    Q_ASSERT(parent);

    QAction *action = this->actions().at(index);
    QString itemId = action->property("itemId").toString();
    parent->getRootMenu()->setItemChecked(itemId, true);
    this->sendItemClickedSignal(itemId, true);

    if (Utils::menuItemCheckableFromId(itemId)) {
        QStringList components = itemId.split(':');
        QString group = components.at(0);

        foreach (QAction *act, this->actions()) {
            QString _itemId = act->property("itemId").toString();

            if (act != action && Utils::menuItemCheckableFromId(_itemId)) {
                QStringList _components = _itemId.split(':');
                QString _group = _components.at(0);
                QString _type = _components.at(1);

                if (_type == "radio" && _group == group) {
                    parent->getRootMenu()->setItemChecked(_itemId, false);
                }
            }
        }
    }

    this->update();
}

void DMenuContent::doUnCheck(int index)
{
    DDockMenu *parent = qobject_cast<DDockMenu*>(this->parent());
    Q_ASSERT(parent);

    QAction *action = this->actions().at(index);
    QString itemId = action->property("itemId").toString();

    if (Utils::menuItemCheckableFromId(itemId)) {
        QStringList components = itemId.split(':');
        QString group = components.at(0);
        QString type = components.at(1);

        if (type == "radio") {
            bool hasNoCheck = true;

            foreach (QAction *act, this->actions()) {
                QString _itemId = act->property("itemId").toString();

                if (Utils::menuItemCheckableFromId(_itemId) && _itemId != itemId) {
                    QStringList components = _itemId.split(':');
                    QString _group = components.at(0);
                    QString _type = components.at(1);

                    if (group == _group && _type == "radio") {
                        QString prop("%1Checked");
                        QVariant checkedCache = parent->getRootMenu()->property(prop.arg(_itemId).toLatin1());
                        bool checked = checkedCache.isNull() ? act->isChecked() : checkedCache.toBool();

                        hasNoCheck = hasNoCheck && !checked;
                    }
                }
            }

            if (!hasNoCheck) {
                parent->getRootMenu()->setItemChecked(itemId, false);
                this->sendItemClickedSignal(action->property("itemId").toString(), false);
            } else {
                parent->getRootMenu()->setItemChecked(itemId, true);
                this->sendItemClickedSignal(action->property("itemId").toString(), true);
            }
        } else {
            parent->getRootMenu()->setItemChecked(itemId, false);
            this->sendItemClickedSignal(action->property("itemId").toString(), false);
        }
    } else {
        parent->getRootMenu()->setItemChecked(itemId, false);
        this->sendItemClickedSignal(action->property("itemId").toString(), false);
    }

    this->update();
}

void DMenuContent::sendItemClickedSignal(QString id, bool checked)
{
    DDockMenu *root = qobject_cast<DDockMenu *>(this->parent());
    Q_ASSERT(root);
    while (root) {
        if (!root->parent()) {
            root->itemClicked(id, checked);
            break;
        } else {
            root = qobject_cast<DDockMenu *>(root->parent());
            Q_ASSERT(root);
        }
    }
}

int DMenuContent::itemIndexUnderEvent(QMouseEvent *event) const
{
    DDockMenu *parent = qobject_cast<DDockMenu*>(this->parent());
    Q_ASSERT(parent);

    DDockMenu *menuUnderCursor = parent->menuUnderPoint(event->globalPos());
    if (menuUnderCursor == parent) {
        int previousHeight = this->rect().y();

        for (int i = 0; i < this->actions().count(); i++) {
            QAction *action = this->actions().at(i);
            int itemHeight = action->text().isEmpty() ? SEPARATOR_HEIGHT : MENU_ITEM_HEIGHT;

            if (previousHeight <= event->y() &&
                    event->y() <= previousHeight + itemHeight &&
                    this->rect().x() <= event->x() &&
                    event->x() <= this->rect().x() + this->rect().width()) {
                return i;
            } else {
                previousHeight += itemHeight;
            }
        }
    }

    return -1;
}

QString DMenuContent::elideText(QString source, int maxWidth) const
{
    QFont font;
    font.setPixelSize(MENU_ITEM_FONT_SIZE);
    QFontMetrics metrics(font);

    if (metrics.width(source) < maxWidth) {
        return source;
    } else {
        return metrics.elidedText(source, Qt::ElideRight, maxWidth);
    }
}
