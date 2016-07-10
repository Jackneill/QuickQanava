//-----------------------------------------------------------------------------
// This file is a part of the FastQMLlibrary. Copyright 2016 Benoit AUTHEMAN.
//
// \file	fqlBottomRightResizer.cpp
// \author	benoit@qanava.org
// \date	2016 07 08
//-----------------------------------------------------------------------------

// Std headers
#include <sstream>

// QT headers
#include <QCursor>
#include <QMouseEvent>

// FQL headers
#include "./fqlBottomRightResizer.h"

namespace fql {  // ::fql

/* BottomRightResizer Object Management *///-----------------------------------
BottomRightResizer::BottomRightResizer( QQuickItem* parent ) :
    QQuickItem( parent )
{
    setFiltersChildMouseEvents( true );

    //setWidth( _handlerSize.width() );   // Set a default size so that mouse event are accepted by default
    //setHeight( _handlerSize.height() );
}

BottomRightResizer::~BottomRightResizer( )
{
    if ( _handler != nullptr )
        _handler->deleteLater();
}
//-----------------------------------------------------------------------------

/* Resizer Management *///-----------------------------------------------------
void    BottomRightResizer::setHandlerComponent( QString handlerComponentString )
{
    _handlerComponentString = handlerComponentString;
    if ( _target != nullptr )   // If there is already a target, force update the target, it will take the new component
        setTarget( _target );   // string into account.
    emit handlerComponentChanged();
}

void    BottomRightResizer::setTarget( QQuickItem* target )
{
    if ( target == nullptr ) {  // Set a null target = disable the control
        if ( _target != nullptr )
            disconnect( _target.data(), 0, this, 0 );  // Disconnect old target width/height monitoring
        _target = nullptr;
        return;
    }

    QQmlEngine* engine = qmlEngine( this );
    if ( engine != nullptr ) {
        _handlerComponent.reset( new QQmlComponent(engine) );
        if ( _handler != nullptr )
            _handler->deleteLater(); // Delete the existing handler
        QString handlerComponentQml = _handlerComponentString;
        if ( handlerComponentQml.isEmpty() ) {  // If no custom QML code has been set for handler component, use a default rectangle
            std::stringstream rectQmlStr; rectQmlStr << "import QtQuick 2.4\n  Rectangle { width:" << _handlerSize.width() <<
                                                        "; height:" << _handlerSize.height() <<
                                                        "; border.width:4; radius:3" <<
                                                        "; border.color:\"" << _handlerColor.name().toStdString() << "\"" <<
                                                        "; color:Qt.rgba(0.,0.,0.,0.); }";
            handlerComponentQml = QString::fromStdString( rectQmlStr.str() );
        }
        _handlerComponent->setData( handlerComponentQml.toUtf8(), QUrl() );
        if ( _handlerComponent->isReady() ) {
            _handler = qobject_cast<QQuickItem*>(_handlerComponent->create());
            if ( _handler != nullptr ) {
                engine->setObjectOwnership( _handler, QQmlEngine::CppOwnership );
                _handler->setOpacity( _autoHideHandler ? 0. : 1. );
                _handler->setSize( _handlerSize );
                QObject* handlerBorder = _handler->property( "border" ).value<QObject*>();
                if ( handlerBorder != nullptr ) {
                    handlerBorder->setProperty( "color", _handlerColor );
                }
                _handler->setVisible( true );
                _handler->setParentItem( this );
                _handler->setAcceptedMouseButtons( Qt::LeftButton );
                _handler->setAcceptHoverEvents( true );
            }
        } else {
            qDebug() << "FastQml: fql::BottomRightResizer::setTarget(): Error: Can't create resize handler QML component:";
            qDebug() << handlerComponentQml;
            qDebug() << "QML Component status=" << _handlerComponent->status();
        }
    }

    _target = target;
    if ( _target != nullptr ) { // Check that target size is not bellow resizer target minimum size
        if ( !_minimumTargetSize.isEmpty() ) {
            if ( _target->width() < _minimumTargetSize.width() )
                _target->setWidth( _minimumTargetSize.width() );
            if ( _target->height() < _minimumTargetSize.height() )
                _target->setHeight( _minimumTargetSize.height() );
        }

        if ( target != parentItem() ) { // Resizer is not in target sibling (ie is not a child of target)
            connect( _target.data(), &QQuickItem::xChanged, this, &BottomRightResizer::onTargetXChanged );
            connect( _target.data(), &QQuickItem::yChanged, this, &BottomRightResizer::onTargetYChanged );
            setX( target->x() );
            setY( target->y() );
        }

        connect( _target.data(), &QQuickItem::widthChanged, this, &BottomRightResizer::onTargetWidthChanged );
        connect( _target.data(), &QQuickItem::heightChanged, this, &BottomRightResizer::onTargetHeightChanged );
    }
    onTargetWidthChanged();
    onTargetHeightChanged();
    emit targetChanged();
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
        //setWidth( targetWidth + handlerWidth2 ); // This item must fit target plus handler br to allow filtering of events on _handler
    }
}

void    BottomRightResizer::onTargetHeightChanged()
{
    if ( _target != nullptr &&
         _handler != nullptr ) {
        qreal targetHeight = _target->height();
        qreal handlerHeight2 = _handlerSize.height() / 2.;
        _handler->setY( targetHeight - handlerHeight2 );
        //setHeight( targetHeight + handlerHeight2 ); // This item must fit target plus handler br to allow filtering of events on _handler
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
bool    BottomRightResizer::childMouseEventFilter( QQuickItem *item, QEvent *event )
{
    if ( _handler != nullptr &&
         item == _handler ) {
        switch ( event->type() ) {
        case QEvent::HoverEnter:
            _handler->setCursor( Qt::SizeFDiagCursor );
            _handler->setOpacity( 1.0 );   // Handler is always visible when hovered
            break;
        case QEvent::HoverLeave:
            _handler->setCursor( Qt::ArrowCursor );
            _handler->setOpacity( getAutoHideHandler() ? 0. : 1.0 );
            break;
        case QEvent::MouseMove: {
            QMouseEvent* me = static_cast<QMouseEvent*>( event );
            if ( me->buttons() |  Qt::LeftButton &&
                 !_dragInitialPos.isNull() &&
                 !_targetInitialSize.isEmpty() ) {
                QPointF delta = ( me->globalPos() - _dragInitialPos );
                if ( _target != nullptr ) {
                    // Do not resize below minimumSize
                    qreal targetWidth = _targetInitialSize.width() + delta.x();
                    if ( targetWidth >= _minimumTargetSize.width() )
                            _target->setWidth( targetWidth );
                    qreal targetHeight = _targetInitialSize.height() + delta.y();
                    if ( targetHeight >= _minimumTargetSize.height() )
                        _target->setHeight( targetHeight );
                }
            }
        }
            break;
        case QEvent::MouseButtonPress: {
            QMouseEvent* me = static_cast<QMouseEvent*>( event );
            if ( _target != nullptr ) {
                _dragInitialPos = me->globalPos();
                _targetInitialSize = { _target->width(), _target->height() };
            }
        }
            break;
        case QEvent::MouseButtonRelease: {
            _dragInitialPos = { 0., 0. };       // Invalid all cached coordinates when button is released
            _targetInitialSize = { 0., 0. };
        }
            break;
        default:
            return false;
        }
        return true;
    }
    return false;
}
//-------------------------------------------------------------------------

} // ::fql
