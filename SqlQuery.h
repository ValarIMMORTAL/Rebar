#pragma once
//查询Components表总数
const char* QUERY_COMPONENTS_COUNT = "select COUNT(SITE) from COMPONENTS";

//查询Components表SITE
const char* QUERY_SITE_DATA = "select SITE,ZONE,STRU from COMPONENTS";

//查询Components表数据
const char* QUERY_COMPONENTS_DATA = "select SITE,ZONE,STRU,COMPONENT,DBTYPE from COMPONENTS";

//查询CSGBox表数据
const char* QUERY_CSGBOX_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency,XLength,YLength,ZLength from CSGBOX";

//查询CSGCylinder表数据
const char* QUERY_CSGCYLINDER_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency,Height,Radius from CSGCYLINDER";

//查询CSGDish表数据
const char* QUERY_CSGDISH_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency,Attribute,Height,Radius from CSGDISH";

const char* QUERY_CSGLINE_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency,startX,startY,startZ,endX,endY,endZ from CSGLINE";

//查询CSGPyramid表数据
const char* QUERY_CSGPYRAMID_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,type,Transform,Translucency,Height,XBottom,YBottom,XOffset,YOffset,XTop,YTop from CSGPyramid";

//查询CSGCtorus表数据
const char* QUERY_CSGCTORUS_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,type,Transform,Translucency,Angle,ArcRadius,SectionRadius from CSGCTorus";

//查询CSGRtorus表数据
const char* QUERY_CSGRTORUS_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,type,Transform,Translucency,Angle,Height,InsideRadius,OutsideRadius from CSGRTorus";

//查询CSGSnout表数据
const char* QUERY_CSGSNOUT_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,type,Transform,Translucency, \
									BottomRadius,BottomXShear,BottomYShear,Height,TopRadius,TopXShear,TopYShear,XAxisDisplacement,YAxisDisplacement from CSGSnout";

//查询CSGLOOP表数据
const char* QUERY_CSGLOOP_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,Id,Normals,Vertices from CSGLoop";

//查询CSGSphere表数据
const char* QUERY_CSGSPHERE_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency,Radius from CSGSphere";

//查询CSGFacetGroup表数据
const char* QUERY_CSGFACETGROUP_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency from CSGFacetGroup";

//查询CSGPolygon表数据
const char* QUERY_CSGPOLYGON_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency from CSGPolygon";

//查询CSGExtrusionEdge表数据
const char* QUERY_CSGEXTRUSIONEDGE_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,Id,DbType,Type,Transform,Bottom,LeftNormal,RightNormal,Top from CSGExtrusionEdge";

//查询Attributes表数据
const char* QUERY_ATTRIBUTES_DATA = "select SITE,ZONE,STRU,ElementId,IsComponent,Name,Type,Value from Attributes";

//查询CSGFloor表数据
const char* QUERY_CSGFLOOR_DATA = "select * from Floor";

//查询CSGVertex表数据
const char* QUERY_CSGVERTEX_DATA = "select * from Vertex";

//查询Grid表数据
const char* QUERY_AxisGrid_DATA = "select * from AxisGrid";

//查询CSGVertex表数据
const char* QUERY_CSGAXISGRID_DATA = "select * from AxisGrid";

//查询wall表数据
const char* QUERY_CSGWALL_DATA = "select * from Wall";

//查询Extrusion表数据
const char* QUERY_EXTRUSION_DATA = "select * from Extrusion";

//查询paragongeometry表数据
const char* QUERY_PARAGONGEOMETRY_DATA = "select * from ParagonGeometry";

//查询Structural表数据
const char* QUERY_STRUCTURAL_DATA = "select * from Structural";

//查询Hole表数据
const char* QUERY_HOLE_DATA = "select * from Hole";

//查询Curve表数据
const char* QUERY_CURVE_DATA = "select * from Curve";

//查询EmbeddedParts表数据
const char* QUERY_EMBEDEDPART_DATA = "select * from EmbeddedParts";

//查询Color表数据
const char* QUERY_COLOR_DATA = "select * from Color";

