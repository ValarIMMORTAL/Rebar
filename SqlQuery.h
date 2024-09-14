#pragma once
//��ѯComponents������
const char* QUERY_COMPONENTS_COUNT = "select COUNT(SITE) from COMPONENTS";

//��ѯComponents��SITE
const char* QUERY_SITE_DATA = "select SITE,ZONE,STRU from COMPONENTS";

//��ѯComponents������
const char* QUERY_COMPONENTS_DATA = "select SITE,ZONE,STRU,COMPONENT,DBTYPE from COMPONENTS";

//��ѯCSGBox������
const char* QUERY_CSGBOX_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency,XLength,YLength,ZLength from CSGBOX";

//��ѯCSGCylinder������
const char* QUERY_CSGCYLINDER_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency,Height,Radius from CSGCYLINDER";

//��ѯCSGDish������
const char* QUERY_CSGDISH_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency,Attribute,Height,Radius from CSGDISH";

const char* QUERY_CSGLINE_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency,startX,startY,startZ,endX,endY,endZ from CSGLINE";

//��ѯCSGPyramid������
const char* QUERY_CSGPYRAMID_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,type,Transform,Translucency,Height,XBottom,YBottom,XOffset,YOffset,XTop,YTop from CSGPyramid";

//��ѯCSGCtorus������
const char* QUERY_CSGCTORUS_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,type,Transform,Translucency,Angle,ArcRadius,SectionRadius from CSGCTorus";

//��ѯCSGRtorus������
const char* QUERY_CSGRTORUS_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,type,Transform,Translucency,Angle,Height,InsideRadius,OutsideRadius from CSGRTorus";

//��ѯCSGSnout������
const char* QUERY_CSGSNOUT_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,type,Transform,Translucency, \
									BottomRadius,BottomXShear,BottomYShear,Height,TopRadius,TopXShear,TopYShear,XAxisDisplacement,YAxisDisplacement from CSGSnout";

//��ѯCSGLOOP������
const char* QUERY_CSGLOOP_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,Id,Normals,Vertices from CSGLoop";

//��ѯCSGSphere������
const char* QUERY_CSGSPHERE_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency,Radius from CSGSphere";

//��ѯCSGFacetGroup������
const char* QUERY_CSGFACETGROUP_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency from CSGFacetGroup";

//��ѯCSGPolygon������
const char* QUERY_CSGPOLYGON_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,DbType,Type,Transform,Translucency from CSGPolygon";

//��ѯCSGExtrusionEdge������
const char* QUERY_CSGEXTRUSIONEDGE_DATA = "select SITE,ZONE,STRU,COMPONENT,Name,Id,DbType,Type,Transform,Bottom,LeftNormal,RightNormal,Top from CSGExtrusionEdge";

//��ѯAttributes������
const char* QUERY_ATTRIBUTES_DATA = "select SITE,ZONE,STRU,ElementId,IsComponent,Name,Type,Value from Attributes";

//��ѯCSGFloor������
const char* QUERY_CSGFLOOR_DATA = "select * from Floor";

//��ѯCSGVertex������
const char* QUERY_CSGVERTEX_DATA = "select * from Vertex";

//��ѯGrid������
const char* QUERY_AxisGrid_DATA = "select * from AxisGrid";

//��ѯCSGVertex������
const char* QUERY_CSGAXISGRID_DATA = "select * from AxisGrid";

//��ѯwall������
const char* QUERY_CSGWALL_DATA = "select * from Wall";

//��ѯExtrusion������
const char* QUERY_EXTRUSION_DATA = "select * from Extrusion";

//��ѯparagongeometry������
const char* QUERY_PARAGONGEOMETRY_DATA = "select * from ParagonGeometry";

//��ѯStructural������
const char* QUERY_STRUCTURAL_DATA = "select * from Structural";

//��ѯHole������
const char* QUERY_HOLE_DATA = "select * from Hole";

//��ѯCurve������
const char* QUERY_CURVE_DATA = "select * from Curve";

//��ѯEmbeddedParts������
const char* QUERY_EMBEDEDPART_DATA = "select * from EmbeddedParts";

//��ѯColor������
const char* QUERY_COLOR_DATA = "select * from Color";

