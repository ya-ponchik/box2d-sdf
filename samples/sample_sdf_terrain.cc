#include "sample.h"

#include "box2d/box2d.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "car.h"
#include "human.h"
#include "random.h"




#define INSIDE_CPP

static float test_time = 0;
static int test_mode = 0;

#include "SDF.h"


static void spawn_sdf(b2WorldId world)
{
	b2BodyDef const body_def = b2DefaultBodyDef();
	b2BodyId const body = b2CreateBody(world, &body_def);
	b2ShapeDef const shape_def = b2DefaultShapeDef();
	SDFTerrainShape shape;

	if (test_mode == 0) {
	
	shape = { sdf_sample_1, { 1.2f, -22.0f },  { 25.0f, 25.0f } };
	create_sdf_terrain_shape(body, &shape_def, &shape);

	shape = { sdf_sample_1, { -42.0f, -7.0f }, { 25.0f, 25.0f } };
	create_sdf_terrain_shape(body, &shape_def, &shape);

	shape = { sdf_sample_1, { 42.0f, -7.0f }, { 25.0f, 25.0f } };
	create_sdf_terrain_shape(body, &shape_def, &shape);

	shape = { sdf_sample_2, sample_2_center, sample_2_half_size };
	create_sdf_terrain_shape(body, &shape_def, &shape);

	shape = { sdf_sample_3, sample_3_center, sample_3_half_size };
	create_sdf_terrain_shape(body, &shape_def, &shape);

	shape = { sdf_sample_4, { 0.0f, 42.5f }, { 1000.0f, 14.0f } };
	create_sdf_terrain_shape(body, &shape_def, &shape);

	shape = { sdf_sample_5, sample_5_center, { 30.0f, 17.0f } };
	create_sdf_terrain_shape(body, &shape_def, &shape);

	shape = { sdf_sample_6, sample_6_center, { 5.0f, 5.0f } };
	create_sdf_terrain_shape(body, &shape_def, &shape);

	} else if (test_mode == 1) {
		shape = { sdf_sample_7, { 0.0f, -500.0f }, { 250.0f, 550.0f } };
		create_sdf_terrain_shape(body, &shape_def, &shape);
	}
}

static void update_sdf()
{
	test_time = (float)glfwGetTime();
}












// This shows how to filter a specific shape using using data.
struct ShapeUserData
{
	int index;
	bool ignore;
};

// Context for ray cast callbacks. Do what you want with this.
struct CastContext
{
	b2Vec2 points[3];
	b2Vec2 normals[3];
	float fractions[3];
	int count;
};

// This callback finds the closest hit. This is the most common callback used in games.
static float RayCastClosestCallback( b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context )
{
	CastContext* rayContext = (CastContext*)context;

	ShapeUserData* userData = (ShapeUserData*)b2Shape_GetUserData( shapeId );

	// Ignore a specific shape. Also NOT ignore initial overlap.
	if ( (userData != nullptr && userData->ignore) /* || fraction == 0.0f  */)
	{
		// By returning -1, we instruct the calling code to ignore this shape and
		// continue the ray-cast to the next shape.
		return -1.0f;
	}

	rayContext->points[0] = point;
	rayContext->normals[0] = normal;
	rayContext->fractions[0] = fraction;
	rayContext->count = 1;

	// By returning the current fraction, we instruct the calling code to clip the ray and
	// continue the ray-cast to the next shape. WARNING: do not assume that shapes
	// are reported in order. However, by clipping, we can always get the closest shape.
	return fraction;
}

// This callback finds any hit. For this type of query we are usually just checking for obstruction,
// so the hit data is not relevant.
// NOTE: shape hits are not ordered, so this may not return the closest hit
static float RayCastAnyCallback( b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context )
{
	CastContext* rayContext = (CastContext*)context;

	ShapeUserData* userData = (ShapeUserData*)b2Shape_GetUserData( shapeId );

	// Ignore a specific shape. Also NOT ignore initial overlap.
	if ( (userData != nullptr && userData->ignore) /* || fraction == 0.0f */ )
	{
		// By returning -1, we instruct the calling code to ignore this shape and
		// continue the ray-cast to the next shape.
		return -1.0f;
	}

	rayContext->points[0] = point;
	rayContext->normals[0] = normal;
	rayContext->fractions[0] = fraction;
	rayContext->count = 1;

	// At this point we have a hit, so we know the ray is obstructed.
	// By returning 0, we instruct the calling code to terminate the ray-cast.
	return 0.0f;
}

// This ray cast collects multiple hits along the ray.
// The shapes are not necessary reported in order, so we might not capture
// the closest shape.
// NOTE: shape hits are not ordered, so this may return hits in any order. This means that
// if you limit the number of results, you may discard the closest hit. You can see this
// behavior in the sample.
static float RayCastMultipleCallback( b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context )
{
	CastContext* rayContext = (CastContext*)context;

	ShapeUserData* userData = (ShapeUserData*)b2Shape_GetUserData( shapeId );

	// Ignore a specific shape. Also NOT ignore initial overlap.
	if ( (userData != nullptr && userData->ignore) /* || fraction == 0.0f */ )
	{
		// By returning -1, we instruct the calling code to ignore this shape and
		// continue the ray-cast to the next shape.
		return -1.0f;
	}

	int count = rayContext->count;
	assert( count < 3 );

	rayContext->points[count] = point;
	rayContext->normals[count] = normal;
	rayContext->fractions[count] = fraction;
	rayContext->count = count + 1;

	if ( rayContext->count == 3 )
	{
		// At this point the buffer is full.
		// By returning 0, we instruct the calling code to terminate the ray-cast.
		return 0.0f;
	}

	// By returning 1, we instruct the caller to continue without clipping the ray.
	return 1.0f;
}

// This ray cast collects multiple hits along the ray and sorts them.
static float RayCastSortedCallback( b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context )
{
	CastContext* rayContext = (CastContext*)context;

	ShapeUserData* userData = (ShapeUserData*)b2Shape_GetUserData( shapeId );
	
	// Ignore a specific shape. Also NOT ignore initial overlap.
	if ( (userData != nullptr && userData->ignore) /* || fraction == 0.0f */ )
	{
		// By returning -1, we instruct the calling code to ignore this shape and
		// continue the ray-cast to the next shape.
		return -1.0f;
	}

	int count = rayContext->count;
	assert( count <= 3 );

	int index = 3;
	while ( fraction < rayContext->fractions[index - 1] )
	{
		index -= 1;

		if ( index == 0 )
		{
			break;
		}
	}

	if ( index == 3 )
	{
		// not closer, continue but tell the caller not to consider fractions further than the largest fraction acquired
		// this only happens once the buffer is full
		assert( rayContext->count == 3 );
		assert( rayContext->fractions[2] <= 1.0f );
		return rayContext->fractions[2];
	}

	for ( int j = 2; j > index; --j )
	{
		rayContext->points[j] = rayContext->points[j - 1];
		rayContext->normals[j] = rayContext->normals[j - 1];
		rayContext->fractions[j] = rayContext->fractions[j - 1];
	}

	rayContext->points[index] = point;
	rayContext->normals[index] = normal;
	rayContext->fractions[index] = fraction;
	rayContext->count = count < 3 ? count + 1 : 3;

	if ( rayContext->count == 3 )
	{
		return rayContext->fractions[2];
	}

	// By returning 1, we instruct the caller to continue without clipping the ray.
	return 1.0f;
}

// This sample shows how to use the ray and shape cast functions on a b2World. This
// sample is configured to NOT ignore initial overlap.
class SDFCastWorld : public Sample
{
public:
	enum Mode
	{
		e_any = 0,
		e_closest = 1,
		e_multiple = 2,
		e_sorted = 3
	};

	enum CastType
	{
		e_rayCast = 0,
		e_circleCast = 1,
		e_capsuleCast = 2,
		e_polygonCast = 3
	};

	enum
	{
		e_maxCount = 64
	};

	explicit SDFCastWorld( SampleContext* context )
		: Sample( context )
	{
		if ( m_context->restart == false )
		{
			m_context->camera.m_center = { 2.0f, 14.0f };
			m_context->camera.m_zoom = 25.0f * 0.75f;
		}

		test_mode = 0;
		
		spawn_sdf(m_worldId);

		// Ground body
		{
			b2BodyDef bodyDef = b2DefaultBodyDef();
			b2BodyId groundId = b2CreateBody( m_worldId, &bodyDef );

			b2ShapeDef shapeDef = b2DefaultShapeDef();
			b2Segment segment = { { -40.0f, 0.0f }, { 40.0f, 0.0f } };
			// b2CreateSegmentShape( groundId, &shapeDef, &segment );
		}

		{
			b2Vec2 vertices[3] = { { -0.5f, 0.0f }, { 0.5f, 0.0f }, { 0.0f, 1.5f } };
			b2Hull hull = b2ComputeHull( vertices, 3 );
			m_polygons[0] = b2MakePolygon( &hull, 0.0f );
		}

		{
			b2Vec2 vertices[3] = { { -0.1f, 0.0f }, { 0.1f, 0.0f }, { 0.0f, 1.5f } };
			b2Hull hull = b2ComputeHull( vertices, 3 );
			m_polygons[1] = b2MakePolygon( &hull, 0.0f );
			m_polygons[1].radius = 0.5f;
		}

		{
			float w = 1.0f;
			float b = w / ( 2.0f + sqrtf( 2.0f ) );
			float s = sqrtf( 2.0f ) * b;

			b2Vec2 vertices[8] = { { 0.5f * s, 0.0f }, { 0.5f * w, b },		 { 0.5f * w, b + s }, { 0.5f * s, w },
								   { -0.5f * s, w },   { -0.5f * w, b + s }, { -0.5f * w, b },	  { -0.5f * s, 0.0f } };

			b2Hull hull = b2ComputeHull( vertices, 8 );
			m_polygons[2] = b2MakePolygon( &hull, 0.0f );
		}

		m_polygons[3] = b2MakeBox( 0.5f, 0.5f );
		m_capsule = { { -0.5f, 0.0f }, { 0.5f, 0.0f }, 0.25f };
		m_circle = { { 0.0f, 0.0f }, 0.5f };
		m_segment = { { -1.0f, 0.0f }, { 1.0f, 0.0f } };

		m_bodyIndex = 0;

		for ( int i = 0; i < e_maxCount; ++i )
		{
			m_bodyIds[i] = b2_nullBodyId;
		}

		m_mode = e_sorted;
		m_ignoreIndex = 7;

		m_castType = e_rayCast;
		m_castRadius = 0.5f;

		m_rayStart = { -20.0f, 10.0f };
		m_rayEnd = { 20.0f, 10.0f };
		m_dragging = false;

		m_angle = 0.0f;
		m_baseAngle = 0.0f;
		m_angleAnchor = { 0.0f, 0.0f };
		m_rotating = false;

		m_simple = false;
	}

	void Create( int index )
	{
		if ( B2_IS_NON_NULL( m_bodyIds[m_bodyIndex] ) )
		{
			b2DestroyBody( m_bodyIds[m_bodyIndex] );
			m_bodyIds[m_bodyIndex] = b2_nullBodyId;
		}

		float x = RandomFloatRange( -20.0f, 20.0f );
		float y = RandomFloatRange( 0.0f, 20.0f );

		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.position = { x, y };
		bodyDef.rotation = b2MakeRot( RandomFloatRange( -B2_PI, B2_PI ) );

		int mod = m_bodyIndex % 3;
		if ( mod == 0 )
		{
			bodyDef.type = b2_staticBody;
		}
		else if ( mod == 1 )
		{
			bodyDef.type = b2_kinematicBody;
		}
		else if ( mod == 2 )
		{
			bodyDef.type = b2_dynamicBody;
			bodyDef.gravityScale = 0.0f;
		}

		m_bodyIds[m_bodyIndex] = b2CreateBody( m_worldId, &bodyDef );

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.userData = m_userData + m_bodyIndex;
		m_userData[m_bodyIndex].ignore = false;
		if ( m_bodyIndex == m_ignoreIndex )
		{
			m_userData[m_bodyIndex].ignore = true;
		}

		if ( index < 4 )
		{
			b2CreatePolygonShape( m_bodyIds[m_bodyIndex], &shapeDef, m_polygons + index );
		}
		else if ( index == 4 )
		{
			b2CreateCircleShape( m_bodyIds[m_bodyIndex], &shapeDef, &m_circle );
		}
		else if ( index == 5 )
		{
			b2CreateCapsuleShape( m_bodyIds[m_bodyIndex], &shapeDef, &m_capsule );
		}
		else
		{
			b2CreateSegmentShape( m_bodyIds[m_bodyIndex], &shapeDef, &m_segment );
		}

		m_bodyIndex = ( m_bodyIndex + 1 ) % e_maxCount;
	}

	void CreateN( int index, int count )
	{
		for ( int i = 0; i < count; ++i )
		{
			Create( index );
		}
	}

	void DestroyBody()
	{
		for ( int i = 0; i < e_maxCount; ++i )
		{
			if ( B2_IS_NON_NULL( m_bodyIds[i] ) )
			{
				b2DestroyBody( m_bodyIds[i] );
				m_bodyIds[i] = b2_nullBodyId;
				return;
			}
		}
	}

	void MouseDown( b2Vec2 p, int button, int mods ) override
	{
		if ( button == GLFW_MOUSE_BUTTON_1 )
		{
			if ( mods == 0 && m_rotating == false )
			{
				m_rayStart = p;
				m_rayEnd = p;
				m_dragging = true;
			}
			else if ( mods == GLFW_MOD_SHIFT && m_dragging == false )
			{
				m_rotating = true;
				m_angleAnchor = p;
				m_baseAngle = m_angle;
			}
		}
	}

	void MouseUp( b2Vec2, int button ) override
	{
		if ( button == GLFW_MOUSE_BUTTON_1 )
		{
			m_dragging = false;
			m_rotating = false;
		}
	}

	void MouseMove( b2Vec2 p ) override
	{
		if ( m_dragging )
		{
			m_rayEnd = p;
		}
		else if ( m_rotating )
		{
			float dx = p.x - m_angleAnchor.x;
			m_angle = m_baseAngle + 1.0f * dx;
		}
	}

	void UpdateGui() override
	{
		float height = 320.0f;
		ImGui::SetNextWindowPos( ImVec2( 10.0f, m_context->camera.m_height - height - 50.0f ), ImGuiCond_Once );
		ImGui::SetNextWindowSize( ImVec2( 200.0f, height ) );

		ImGui::Begin( "Ray-cast World", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize );

		// ImGui::Checkbox( "Simple", &m_simple );

		if ( m_simple == false )
		{
			const char* castTypes[] = { "Ray", "Circle", "Capsule", "Polygon" };
			int castType = int( m_castType );
			/*
			if ( ImGui::Combo( "Type", &castType, castTypes, IM_ARRAYSIZE( castTypes ) ) )
			{
				m_castType = CastType( castType );
			}
			*/

			if ( m_castType != e_rayCast )
			{
				ImGui::SliderFloat( "Radius", &m_castRadius, 0.0f, 2.0f, "%.1f" );
			}

			const char* modes[] = { "Any", "Closest", "Multiple", "Sorted" };
			int mode = int( m_mode );
			if ( ImGui::Combo( "Mode", &mode, modes, IM_ARRAYSIZE( modes ) ) )
			{
				m_mode = Mode( mode );
			}
		}

		if ( ImGui::Button( "Polygon 1" ) )
			Create( 0 );
		ImGui::SameLine();
		if ( ImGui::Button( "10x##Poly1" ) )
			CreateN( 0, 10 );

		if ( ImGui::Button( "Polygon 2" ) )
			Create( 1 );
		ImGui::SameLine();
		if ( ImGui::Button( "10x##Poly2" ) )
			CreateN( 1, 10 );

		if ( ImGui::Button( "Polygon 3" ) )
			Create( 2 );
		ImGui::SameLine();
		if ( ImGui::Button( "10x##Poly3" ) )
			CreateN( 2, 10 );

		if ( ImGui::Button( "Box" ) )
			Create( 3 );
		ImGui::SameLine();
		if ( ImGui::Button( "10x##Box" ) )
			CreateN( 3, 10 );

		if ( ImGui::Button( "Circle" ) )
			Create( 4 );
		ImGui::SameLine();
		if ( ImGui::Button( "10x##Circle" ) )
			CreateN( 4, 10 );

		if ( ImGui::Button( "Capsule" ) )
			Create( 5 );
		ImGui::SameLine();
		if ( ImGui::Button( "10x##Capsule" ) )
			CreateN( 5, 10 );

		if ( ImGui::Button( "Segment" ) )
			Create( 6 );
		ImGui::SameLine();
		if ( ImGui::Button( "10x##Segment" ) )
			CreateN( 6, 10 );

		if ( ImGui::Button( "Destroy Shape" ) )
		{
			DestroyBody();
		}

		ImGui::End();
	}

	void Step() override
	{
		update_sdf();

		Sample::Step();

		DrawTextLine( "Click left mouse button and drag to modify ray cast" );
		DrawTextLine( "Shape 7 is intentionally ignored by the ray" );

		b2HexColor color1 = b2_colorGreen;
		b2HexColor color2 = b2_colorLightGray;
		b2HexColor color3 = b2_colorMagenta;

		b2Vec2 rayTranslation = b2Sub( m_rayEnd, m_rayStart );

		m_context->draw.DrawPoint( m_rayStart, 5.0f, b2_colorGreen );

		if ( m_simple )
		{
			DrawTextLine( "Simple closest point ray cast" );

			// This version doesn't have a callback, but it doesn't skip the ignored shape
			b2RayResult result = b2World_CastRayClosest( m_worldId, m_rayStart, rayTranslation, b2DefaultQueryFilter() );

			if ( result.hit == true && result.fraction > 0.0f )
			{
				b2Vec2 c = b2MulAdd( m_rayStart, result.fraction, rayTranslation );
				m_context->draw.DrawPoint( result.point, 5.0f, color1 );
				m_context->draw.DrawSegment( m_rayStart, c, color2 );
				b2Vec2 head = b2MulAdd( result.point, 0.5f, result.normal );
				m_context->draw.DrawSegment( result.point, head, color3 );
			}
			else
			{
				m_context->draw.DrawSegment( m_rayStart, m_rayEnd, color2 );
			}
		}
		else
		{
			switch ( m_mode )
			{
				case e_any:
					DrawTextLine( "Cast mode: any - check for obstruction - unsorted" );
					break;

				case e_closest:
					DrawTextLine( "Cast mode: closest - find closest shape along the cast" );
					break;

				case e_multiple:
					DrawTextLine( "Cast mode: multiple - gather up to 3 shapes - unsorted" );
					break;

				case e_sorted:
					DrawTextLine( "Cast mode: sorted - gather up to 3 shapes sorted by closeness" );
					break;

				default:
					assert( false );
					break;
			}

			b2CastResultFcn* functions[] = {
				RayCastAnyCallback,
				RayCastClosestCallback,
				RayCastMultipleCallback,
				RayCastSortedCallback,
			};
			b2CastResultFcn* modeFcn = functions[m_mode];

			CastContext context = {};

			// Must initialize fractions for sorting
			context.fractions[0] = FLT_MAX;
			context.fractions[1] = FLT_MAX;
			context.fractions[2] = FLT_MAX;

			b2Transform transform = { m_rayStart, b2MakeRot( m_angle ) };
			b2Circle circle = { .center = m_rayStart, .radius = m_castRadius };
			b2Capsule capsule = { b2TransformPoint( transform, { -0.25f, 0.0f } ), b2TransformPoint( transform, { 0.25f, 0.0f } ),
								  m_castRadius };
			b2Polygon box = b2MakeOffsetRoundedBox( 0.25f, 0.5f, transform.p, transform.q, m_castRadius );
			b2ShapeProxy proxy = {};

			if ( m_castType == e_rayCast )
			{
				b2World_CastRay( m_worldId, m_rayStart, rayTranslation, b2DefaultQueryFilter(), modeFcn, &context );
			}
			else
			{
				if ( m_castType == e_circleCast )
				{
					proxy = b2MakeProxy( &circle.center, 1, circle.radius );
				}
				else if ( m_castType == e_capsuleCast )
				{
					proxy = b2MakeProxy( &capsule.center1, 2, capsule.radius );
				}
				else
				{
					proxy = b2MakeProxy( box.vertices, box.count, box.radius );
				}

				b2World_CastShape( m_worldId, &proxy, rayTranslation, b2DefaultQueryFilter(), modeFcn, &context );
			}

			if ( context.count > 0 )
			{
				assert( context.count <= 3 );
				b2HexColor colors[3] = { b2_colorRed, b2_colorGreen, b2_colorBlue };
				for ( int i = 0; i < context.count; ++i )
				{
					b2Vec2 c = b2MulAdd( m_rayStart, context.fractions[i], rayTranslation );
					b2Vec2 p = context.points[i];
					b2Vec2 n = context.normals[i];
					m_context->draw.DrawPoint( p, 5.0f, colors[i] );
					m_context->draw.DrawSegment( m_rayStart, c, color2 );
					b2Vec2 head = b2MulAdd( p, 1.0f, n );
					m_context->draw.DrawSegment( p, head, color3 );

					b2Vec2 t = b2MulSV( context.fractions[i], rayTranslation );
					b2Transform shiftedTransform = { t, b2Rot_identity };

					if ( m_castType == e_circleCast )
					{
						m_context->draw.DrawSolidCircle( shiftedTransform, circle.center, m_castRadius, b2_colorYellow );
					}
					else if ( m_castType == e_capsuleCast )
					{
						b2Vec2 p1 = capsule.center1 + t;
						b2Vec2 p2 = capsule.center2 + t;
						m_context->draw.DrawSolidCapsule( p1, p2, m_castRadius, b2_colorYellow );
					}
					else if ( m_castType == e_polygonCast )
					{
						m_context->draw.DrawSolidPolygon( shiftedTransform, box.vertices, box.count, box.radius, b2_colorYellow );
					}
				}
			}
			else
			{
				b2Transform shiftedTransform = { b2Add( transform.p, rayTranslation ), transform.q };
				m_context->draw.DrawSegment( m_rayStart, m_rayEnd, color2 );

				if ( m_castType == e_circleCast )
				{
					m_context->draw.DrawSolidCircle( shiftedTransform, b2Vec2_zero, m_castRadius, b2_colorGray );
				}
				else if ( m_castType == e_capsuleCast )
				{
					b2Vec2 p1 = b2Add( b2TransformPoint( transform, capsule.center1 ), rayTranslation );
					b2Vec2 p2 = b2Add( b2TransformPoint( transform, capsule.center2 ), rayTranslation );
					m_context->draw.DrawSolidCapsule( p1, p2, m_castRadius, b2_colorYellow );
				}
				else if ( m_castType == e_polygonCast )
				{
					m_context->draw.DrawSolidPolygon( shiftedTransform, box.vertices, box.count, box.radius, b2_colorYellow );
				}
			}
		}

		if ( B2_IS_NON_NULL( m_bodyIds[m_ignoreIndex] ) )
		{
			b2Vec2 p = b2Body_GetPosition( m_bodyIds[m_ignoreIndex] );
			p.x -= 0.2f;
			m_context->draw.DrawString( p, "ign" );
		}
	}

	static Sample* Create( SampleContext* context )
	{
		return new SDFCastWorld( context );
	}

	int m_bodyIndex;
	b2BodyId m_bodyIds[e_maxCount] = {};
	ShapeUserData m_userData[e_maxCount] = {};
	b2Polygon m_polygons[4] = {};
	b2Capsule m_capsule;
	b2Circle m_circle;
	b2Segment m_segment;

	bool m_simple;

	int m_mode;
	int m_ignoreIndex;

	CastType m_castType;
	float m_castRadius;

	b2Vec2 m_angleAnchor;
	float m_baseAngle;
	float m_angle;
	bool m_rotating;

	b2Vec2 m_rayStart;
	b2Vec2 m_rayEnd;
	bool m_dragging;
};























class SDFCollision : public Sample {
public:
	static Sample* create(SampleContext* context)
	{
		return new SDFCollision(*context);
	}
private:
	explicit SDFCollision(SampleContext& context)
		: Sample(&context)
	{
		m_camera->m_center = { 8.0f, 30.0f };
		m_camera->m_zoom = 60.0f;

		test_mode = 0;

		spawn_sdf(m_worldId);

		m_car.Spawn(m_worldId, { 43.0f, 56.5f }, 3.0f, 5.0f, 0.7f, 85.0f, nullptr);

		{
			Human h { 0 };
			CreateHuman(&h, m_worldId, { -42.0f, 35.0f }, 3.5f, 0.05f, 5.0f, 0.5f, 99999, nullptr, false);
		}

		b2BodyDef body_def = b2DefaultBodyDef();
		body_def.type = b2_dynamicBody;
		b2ShapeDef shape_def = b2DefaultShapeDef();

		shape_def.material.rollingResistance = 0.15f;
		
		for (int i = 0; i < 5; ++i) {
			body_def.position = { -10.0f, 20.0f };
			bool const is_simple = false;
			b2Circle const c { {}, i == 0 ? 5.0f : 1.0f, is_simple };
			b2CreateCircleShape(b2CreateBody(m_worldId, &body_def), &shape_def, &c);
		}

		shape_def.material.rollingResistance = 0;

		for (int i = 0; i < 5; ++i) {
			body_def.position = { 13.0f, 10.0f };
			auto const length = RandomFloatRange(0.65f, 2.5f);
			b2Capsule const capsule { { 0.0f, -0.5f * length }, { 0.0f, 0.5f * length }, RandomFloatRange(0.65f, 1.25f) };
			b2CreateCapsuleShape(b2CreateBody(m_worldId, &body_def), &shape_def, &capsule);	
		}

		{
			body_def.position = { 0.0f, 15.0f };
			auto const body = b2CreateBody(m_worldId, &body_def);

			b2Vec2 vertices[3];
			vertices[0] = { -1.0f, 0.0f };
			vertices[1] = { 0.5f, 1.0f };
			vertices[2] = { 0.0f, 2.0f };
			b2Hull hull = b2ComputeHull( vertices, 3 );
			b2Polygon left = b2MakePolygon( &hull, 0.0f );

			vertices[0] = { 1.0f, 0.0f };
			vertices[1] = { -0.5f, 1.0f };
			vertices[2] = { 0.0f, 2.0f };
			hull = b2ComputeHull( vertices, 3 );
			b2Polygon right = b2MakePolygon( &hull, 0.0f );

			b2CreatePolygonShape(body, &shape_def, &left);
			b2CreatePolygonShape(body, &shape_def, &right);
		}

		for (int i = 0; i < 5; ++i) {
			body_def.position = { -10.0f, 38.0f };
			b2Polygon const box = b2MakeBox(RandomFloatRange(0.1f, 2.5f), RandomFloatRange(0.5f, 3.75f));
			b2CreatePolygonShape(b2CreateBody(m_worldId, &body_def), &shape_def, &box);
		}
	}

	void Step() override
	{
		update_sdf();

		if (glfwGetKey(m_context->window, GLFW_KEY_A) == GLFW_PRESS)
			m_car.SetSpeed(40.0f);
		if (glfwGetKey(m_context->window, GLFW_KEY_S) == GLFW_PRESS)
			m_car.SetSpeed(0.0f);
		if (glfwGetKey(m_context->window, GLFW_KEY_D) == GLFW_PRESS)
			m_car.SetSpeed(-40.0f);

		DrawTextLine("Car: left = A, brake = S, right = D");

		Sample::Step();
	}

	Car m_car;
};











class SDFProcedural : public Sample {
public:
	static Sample* create(SampleContext* context)
	{
		return new SDFProcedural(*context);
	}
private:
	explicit SDFProcedural(SampleContext& context)
		: Sample(&context)
	{
		m_camera->m_center = { 8.0f, -12.0f };
		m_camera->m_zoom = 60.0f;

		test_mode = 1;

		spawn_sdf(m_worldId);
		
		b2BodyDef body_def = b2DefaultBodyDef();
		body_def.type = b2_dynamicBody;
		body_def.position = { -10.0f, 20.0f };
		b2ShapeDef shape_def = b2DefaultShapeDef();
		shape_def.material.rollingResistance = 0.15f;
		b2Circle const c { {}, 5.0f };
		b2CreateCircleShape(b2CreateBody(m_worldId, &body_def), &shape_def, &c);
	}
};














static int sample_1 = RegisterSample("SDF Terrain", "SDF Ray Cast World", SDFCastWorld::Create);
static int sample_2 = RegisterSample("SDF Terrain", "SDF Collision", SDFCollision::create);
static int sample_3 = RegisterSample("SDF Terrain", "SDF Procedural", SDFProcedural::create);
