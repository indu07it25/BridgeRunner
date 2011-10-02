//
//  Scene.cpp
//  BridgeRunner
//
//  Created by Ole Andre Birkedal on 9/6/11.
//  Copyright 2011 Extab. All rights reserved.
//

#include <iostream>
#include "BridgeSegment.h"
#include "Scene.h"

Scene::Scene()
{
    setIsTouchEnabled( true );
    setIsAccelerometerEnabled( false );
    
    ScreenSize = CCDirector::sharedDirector()->getWinSize();
    
    schedule( schedule_selector( Scene::tick ) );
    
    // Create the physics object
    World = new Physics();
    
    BridgeLengthEpsilon = 10e-3;
    
    IterationCnt = 0;
    
    ScreenTouched = false;
}

Scene::~Scene()
{
    delete World;
    World = NULL;
}

void Scene::draw()
{
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glLineWidth( 2.0 );
    World->RenderAll();

    if( ScreenTouched )
    {
        // Draw the bridge indication
        glColor4f( 1.0, 0.0, 0.0, 1.0 );
        ccDrawLine( BoxToCCVec( SegmentStart ), BoxToCCVec( SegmentStop ) );
    }
    
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

b2Vec2 Scene::CCToBoxVec( cocos2d::CCPoint Point )
{
    return b2Vec2( Point.x, Point.y );
}

CCPoint Scene::BoxToCCVec( b2Vec2 Point )
{
    return CCPoint( Point.x, Point.y );
}

void Scene::tick( cocos2d::ccTime dt )
{
    IterationCnt++;
    
    // This is being reset every time the user touches the screen,
    // user for double click logic
    IterationsSinceLastClick++;

    World->DoStep();
}

void Scene::ccTouchesMoved( CCSet *touches, CCEvent *event )
{
    CCSetIterator it;
    CCTouch *touch;
    
    // FIXME: use anyObject() or begin()?
    touch = static_cast< CCTouch * >( touches->anyObject() );
    
    if( !touch )
        return;
    
    CCPoint location = touch->locationInView( touch->view() );
    location = CCDirector::sharedDirector()->convertToGL( location );

    SegmentStart = StartTouch;
    SegmentStop  = CCToBoxVec( location );
}

void Scene::ccTouchesBegan( cocos2d::CCSet *touches, cocos2d::CCEvent *event )
{
    CCSetIterator it;
    CCTouch *touch;

    // FIXME: for_each and lambda with C++11
    for( it = touches->begin(); it != touches->end(); ++it )
    {
        touch = static_cast< CCTouch * >( *it );
        
        if( !touch )
            break;
        
        // Set flag to indicate that screen is currently being touched
        ScreenTouched = true;
        
        CCPoint location = touch->locationInView( touch->view() );
        location = CCDirector::sharedDirector()->convertToGL( location );
        
        StartTouch = CCToBoxVec( location );
        
        // If user clicked twice in 30 iterations it is considered a double-click
        // FIXME: generalize this functionality
        if( IterationsSinceLastClick < 30 )
        {
            DelBridgeSegment( StartTouch );
        }
        
        IterationsSinceLastClick = 0;
    }
}

bool Scene::AddBridgeSegment( b2Vec2 Start, b2Vec2 Stop )
{
    b2Vec2 StartStopDiff = Start - Stop;

    // Do not add bridge segment if it is too small. We do not want tiny
    // 5-10 pixel size bridge segments, difficult to delete.
    if( b2Dot( StartStopDiff, StartStopDiff ) < BridgeLengthEpsilon )
        return false;
    
    BridgeSegment *NewSegment = new BridgeSegment( Start, Stop );
    
    World->AddPhyObj( NewSegment );
    BridgeSegments.push_back( NewSegment );
    
    return true;
}

bool Scene::DelBridgeSegment( b2Vec2 Position )
{
    PhyObj *Selected = World->GetObjAtPosition( Position );
    //BridgeSegment *Segment = dynamic_cast< BridgeSegment * >( Selected );
    
    // Selected object is not a bridge segment
//    if( !Segment )
//        return false;
    
    if( Selected )
        std::cout << "Info: " << Selected->GetPosition().x << " " << Selected->GetPosition().y << std::endl;
    //std::cout << "Info: " << Segment->GetPosition().x << " " << Segment->GetPosition().y << std::endl;
    
    // Remove the oldest bridge segment
    BridgeSegments.pop_front();
    
    return true;
}

void Scene::ccTouchesEnded( cocos2d::CCSet *touches, cocos2d::CCEvent* event )
{
	CCSetIterator it;
	CCTouch *touch;

    // FIXME: for_each and lambda with C++11
	for( it = touches->begin(); it != touches->end(); ++it )
	{
        touch = static_cast< CCTouch * >( *it );
        
		if( !touch )
			break;
        
        // Finger released from screen, no longer touched
        ScreenTouched = false;
        
        CCPoint location = touch->locationInView( touch->view() );
		location = CCDirector::sharedDirector()->convertToGL( location );
        
        if( b2Dot( SegmentStart - SegmentStop, SegmentStart - SegmentStop ) > 50.0 )
        {
            World->AddPhyObj( new BridgeSegment( SegmentStart, SegmentStop ) );
        }
        
	}
}

CCScene *Scene::scene()
{
    CCScene *scene = CCScene::node();
    CCLayer *layer = new Scene();
    scene->addChild( layer );
    layer->release();
    
    return scene;
}