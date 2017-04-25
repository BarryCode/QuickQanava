/*
    This file is part of QuickQanava library.

    Copyright (C) 2008-2017 Benoit AUTHEMAN

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickQanava software library.
//
// \file	qanBottomRightResizer.cpp
// \author	benoit@destrat.io
// \date	2016 07 08
//-----------------------------------------------------------------------------

// Std headers
#include <sstream>

// Qt headers
#include <QCursor>
#include <QMouseEvent>

// QuickQanava headers
#include "./qanBottomRightResizer.h"

namespace qan {  // ::qan

/* BottomRightResizer Object Management *///-----------------------------------
BottomRightResizer::BottomRightResizer( QQuickItem* parent ) :
    QQuickItem( parent )
{
}

BottomRightResizer::~BottomRightResizer( )
{
    if ( _handler != nullptr )
        _handler->deleteLater();
}
//-----------------------------------------------------------------------------

/* Resizer Management *///-----------------------------------------------------
void    BottomRightResizer::setHandler( QQuickItem* handler ) noexcept
{
    if ( handler != _handler.data() ) {
        if ( _handler ) {
            if ( QQmlEngine::objectOwnership(_handler.data()) == QQmlEngine::CppOwnership )
                _handler.data()->deleteLater();
        }
        _handler = handler;
        if ( _handler )
            _handler->installEventFilter(this);
        emit handlerChanged();
    }
    if ( _target )      // Force target reconfiguration for new handler
        configureTarget(*_target);
}

QQuickItem* BottomRightResizer::getHandler( ) const noexcept
{
    return ( _handler ? _handler.data() : nullptr );
}

void    BottomRightResizer::setTarget( QQuickItem* target )
{
    if ( target == nullptr ) {  // Set a null target = disable the control
        if ( _target != nullptr )
            disconnect( _target.data(), 0, this, 0 );  // Disconnect old target width/height monitoring
        _target = nullptr;
        return;
    }

    if ( _handler == nullptr ) {
        QQmlEngine* engine = qmlEngine( this );
        if ( engine != nullptr ) {
            QQmlComponent defaultHandlerComponent{engine};
            QString handlerQml{ QStringLiteral("import QtQuick 2.7\n  Rectangle {") +
                        QStringLiteral("width:")    + QString::number(_handlerSize.width()) + QStringLiteral(";") +
                        QStringLiteral("height:")   + QString::number(_handlerSize.height()) + QStringLiteral(";") +
                        QStringLiteral("border.width:4;radius:3;") +
                        QStringLiteral("border.color:\"") + _handlerColor.name() + QStringLiteral("\";") +
                        QStringLiteral("color:Qt.lighter(border.color); }") };
            defaultHandlerComponent.setData( handlerQml.toUtf8(), QUrl{} );
            if ( defaultHandlerComponent.isReady() ) {
                _handler = qobject_cast<QQuickItem*>(defaultHandlerComponent.create());
                if ( _handler ) {
                    engine->setObjectOwnership( _handler.data(), QQmlEngine::CppOwnership );
                    _handler->setParentItem(this);
                    _handler->installEventFilter(this);
                }
                else {
                    qWarning() << "FastQml: fql::BottomRightResizer::setTarget(): Error: Can't create resize handler QML component:";
                    qWarning() << "QML Component status=" << defaultHandlerComponent.status();
                }
            }
        }
    }

    // Configure handler on given target
    if ( _handler != nullptr )
        configureHandler(*_handler);

    _target = target;
    if ( _target )
        configureTarget(*_target);
    emit targetChanged();
}

void    BottomRightResizer::configureHandler(QQuickItem& handler) noexcept
{
    handler.setOpacity( _autoHideHandler ? 0. : 1. );
    handler.setSize( _handlerSize );
    handler.setZ( z() + 1. );
    QObject* handlerBorder = handler.property( "border" ).value<QObject*>();
    if ( handlerBorder != nullptr )
        handlerBorder->setProperty( "color", _handlerColor );
    handler.setVisible( true );
    handler.setParentItem( this );
    handler.setAcceptedMouseButtons( Qt::LeftButton );
    handler.setAcceptHoverEvents( true );
}

void    BottomRightResizer::configureTarget(QQuickItem& target) noexcept
{
    if ( !_minimumTargetSize.isEmpty() ) { // Check that target size is not bellow resizer target minimum size
        if ( target.width() < _minimumTargetSize.width() )
            target.setWidth( _minimumTargetSize.width() );
        if ( target.height() < _minimumTargetSize.height() )
            target.setHeight( _minimumTargetSize.height() );
    }

    if ( &target != parentItem() ) { // Resizer is not in target sibling (ie is not a child of target)
        connect( &target,   &QQuickItem::xChanged,
                 this,      &BottomRightResizer::onTargetXChanged );
        connect( &target,   &QQuickItem::yChanged,
                 this,      &BottomRightResizer::onTargetYChanged );
        setX( target.x() );
        setY( target.y() );
    }

    connect( &target,   &QQuickItem::widthChanged,
             this,      &BottomRightResizer::onTargetWidthChanged );
    connect( &target,   &QQuickItem::heightChanged,
             this,      &BottomRightResizer::onTargetHeightChanged );

    onTargetWidthChanged();
    onTargetHeightChanged();
}

void    BottomRightResizer::onTargetXChanged()
{
    if ( _target != nullptr &&
         _target != parentItem() )
        setX( _target->x() );
}

void    BottomRightResizer::onTargetYChanged()
{
    if ( _target != nullptr &&
         _target != parentItem() )
        setY( _target->y() );
}

void    BottomRightResizer::onTargetWidthChanged()
{
    if ( _target != nullptr &&
         _handler != nullptr ) {
        qreal targetWidth = _target->width();
        qreal handlerWidth2 = _handlerSize.width() / 2.;
        _handler->setX( targetWidth - handlerWidth2 );
    }
}

void    BottomRightResizer::onTargetHeightChanged()
{
    if ( _target != nullptr &&
         _handler != nullptr ) {
        qreal targetHeight = _target->height();
        qreal handlerHeight2 = _handlerSize.height() / 2.;
        _handler->setY( targetHeight - handlerHeight2 );
    }
}

void    BottomRightResizer::setHandlerSize( QSizeF handlerSize )
{
    if ( handlerSize.isEmpty() )
        return;
    if ( handlerSize == _handlerSize )  // Binding loop protection
        return;

    _handlerSize = handlerSize;
    if ( _handler != nullptr ) {
        onTargetWidthChanged(); // Force resize handler position change
        onTargetHeightChanged();    // to take new handler size

        _handler->setSize( handlerSize );
    }

    emit handlerSizeChanged();
}

void    BottomRightResizer::setHandlerColor( QColor handlerColor )
{
    if ( !handlerColor.isValid() )
        return;
    if ( handlerColor == _handlerColor )    // Binding loop protection
        return;
    if ( _handler != nullptr ) {
        QObject* handlerBorder = _handler->property( "border" ).value<QObject*>();
        if ( handlerBorder != nullptr ) {
            handlerBorder->setProperty( "color", handlerColor );
        }
    }
    _handlerColor = handlerColor;
    emit handlerColorChanged();
}

void    BottomRightResizer::setHandlerRadius( qreal handlerRadius )
{
    if ( qFuzzyCompare( 1.0 + handlerRadius, 1.0 + _handlerRadius ) )    // Binding loop protection
        return;
    if ( _handler != nullptr )
        _handler->setProperty( "radius", handlerRadius );
    _handlerRadius = handlerRadius;
    emit handlerRadiusChanged();
}

void    BottomRightResizer::setHandlerWidth( qreal handlerWidth )
{
    if ( qFuzzyCompare( 1.0 + handlerWidth, 1.0 + _handlerWidth ) )    // Binding loop protection
        return;
    if ( _handler != nullptr ) {
        QObject* handlerBorder = _handler->property( "border" ).value<QObject*>();
        if ( handlerBorder != nullptr ) {
            handlerBorder->setProperty( "width", handlerWidth );
        }
    }
    _handlerWidth = handlerWidth;
    emit handlerWidthChanged();
}
void    BottomRightResizer::setMinimumTargetSize( QSizeF minimumTargetSize )
{
    if ( minimumTargetSize.isEmpty() )
        return;
    if ( _target != nullptr ) { // Eventually, resize target if its actual size is below minimum
        if ( _target->width() < minimumTargetSize.width() )
            _target->setWidth( minimumTargetSize.width() );
        if ( _target->height() < minimumTargetSize.height() )
            _target->setHeight( minimumTargetSize.height() );
    }
    _minimumTargetSize = minimumTargetSize;
    emit minimumTargetSizeChanged();
}

void    BottomRightResizer::setAutoHideHandler( bool autoHideHandler )
{
    if ( autoHideHandler == _autoHideHandler )    // Binding loop protection
        return;
    if ( _handler != nullptr &&
         autoHideHandler &&
         _handler->isVisible() )    // If autoHide is set to false and the handler is visible, hide it
        _handler->setVisible( false );
    _autoHideHandler = autoHideHandler;
    emit autoHideHandlerChanged();
}
//-----------------------------------------------------------------------------

/* Resizer Management *///-----------------------------------------------------
bool   BottomRightResizer::eventFilter(QObject *item, QEvent *event)
{
    if ( item != _handler )
        return QObject::eventFilter(item, event);
    bool accepted{ false };
    if ( _handler != nullptr &&
         item == _handler.data() ) {
        switch ( event->type() ) {
        case QEvent::HoverEnter:
        {
            _handler->setCursor( Qt::SizeFDiagCursor );
            _handler->setOpacity( 1.0 );   // Handler is always visible when hovered
            QMouseEvent* me = static_cast<QMouseEvent*>( event );
            me->setAccepted(true);
            accepted = true;
        }
            break;
        case QEvent::HoverLeave:
        {
            _handler->setCursor( Qt::ArrowCursor );
            _handler->setOpacity( getAutoHideHandler() ? 0. : 1.0 );
            QMouseEvent* me = static_cast<QMouseEvent*>( event );
            me->setAccepted(true);
            accepted = true;
            break;
        }
        case QEvent::MouseMove: {
            QMouseEvent* me = static_cast<QMouseEvent*>( event );
            if ( me->buttons() |  Qt::LeftButton &&
                 !_dragInitialPos.isNull() &&
                 !_targetInitialSize.isEmpty() ) {
                // Inspired by void QQuickMouseArea::mouseMoveEvent(QMouseEvent *event)
                // https://code.woboq.org/qt5/qtdeclarative/src/quick/items/qquickmousearea.cpp.html#47curLocalPos
                // Coordinate mapping in qt quick is even more a nightmare than with graphics view...
                // BTW, this code is probably buggy for deep quick item hierarchy.
                QPointF startLocalPos;
                QPointF curLocalPos;
                if ( parentItem() != nullptr ) {
                    startLocalPos = parentItem()->mapFromScene( _dragInitialPos );
                    curLocalPos = parentItem()->mapFromScene( me->windowPos() );
                } else {
                    startLocalPos = _dragInitialPos;
                    curLocalPos = me->windowPos();
                }
                QPointF delta{ curLocalPos - startLocalPos };
                if ( _target != nullptr ) {
                    // Do not resize below minimumSize
                    qreal targetWidth = _targetInitialSize.width() + delta.x();
                    if ( targetWidth >= _minimumTargetSize.width() )
                            _target->setWidth( targetWidth );
                    qreal targetHeight = _targetInitialSize.height() + delta.y();
                    if ( targetHeight >= _minimumTargetSize.height() )
                        _target->setHeight( targetHeight );
                    me->setAccepted(true);
                    accepted = true;
                }
            }
        }
            break;
        case QEvent::MouseButtonPress: {
            QMouseEvent* me = static_cast<QMouseEvent*>( event );
            if ( _target != nullptr ) {
                _dragInitialPos = me->windowPos();
                _targetInitialSize = { _target->width(), _target->height() };
                emit resizeStart( _target != nullptr ? QSizeF{ _target->width(), _target->height() } :
                                                       QSizeF{} );
                if ( getFlickable() != nullptr )
                    getFlickable()->setProperty( "interactive", QVariant{false} );
                me->setAccepted(true);
                accepted = true;
            }
        }
            break;
        case QEvent::MouseButtonRelease: {
            _dragInitialPos = { 0., 0. };       // Invalid all cached coordinates when button is released
            _targetInitialSize = { 0., 0. };
            emit resizeEnd( _target != nullptr ? QSizeF{ _target->width(), _target->height() } :
                                                 QSizeF{} );
            if ( getFlickable() != nullptr )
                getFlickable()->setProperty( "interactive", QVariant{true} );
        }
            break;
        default:
            accepted = false;
        }
    }
    return accepted ? true : QObject::eventFilter(item, event);
}
//-------------------------------------------------------------------------

} // ::qan
