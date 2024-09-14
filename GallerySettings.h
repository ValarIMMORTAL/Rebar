#pragma once

#include "CommonFile.h"

namespace Gallery
{
	/// @brief 端部的类型
	/// @details 与g_listEndType对应
	enum class EndType : int
	{
		/// @brief 无
		None = 0,
		/// @brief 弯曲
		Bend = 1,
		/// @brief 吊钩
		Hook = 2,
		/// @brief 折线
		PolygonalLine = 3,
		/// @brief 90度弯钩
		Hook90 = 4,
		/// @brief 135度弯钩
		Hook135 = 5,
		/// @brief 180度弯钩
		Hook180 = 6,
		/// @brief 直锚
		Straight = 7,
		/// @brief 用户自定义
		Custom = 8,
		/// @brief 自定义弯锚
		/// @details 可以在任意角度上弯曲
		CustomBend = 9,
	};

	/// @brief 直锚参数
	struct EndTypeDataStraight {
		/// @brief 锚入长度
		double length;
	};

	/// @brief 不同度数的弯钩的参数
	struct EndTypeDataHookDegree {
		/// @brief 弯曲半径
		double radius;
		/// @brief 预留长度
		/// @details 应该是弯曲后的长度
		double reserver_length;
	};

	/// @brief 自定义弯锚
	struct EndTypeCustomBend{
		/// @brief 弯曲角度 单位deg
		/// @details 这个角度是钢筋外部的夹角，如下图所示
		///      \ 
		///       \ <--here
		///--------...........
		/// 钢筋     延长线
		double angle;
		/// @brief 弯曲半径
		double radius;
		/// @brief 弯曲长度
		double length;
	};


	/// @brief 不同端部类型的参数
	/// @details 除以下定义的参数以外，其余类型没有参数
	union EndTypeData {
		/// @brief 类型为直锚时的参数
		EndTypeDataStraight straight;
		/// @brief 类型为带度数的弯钩时的参数
		EndTypeDataHookDegree hook_degree;
		/// @brief 自定义弯锚
		EndTypeCustomBend custom_bend;
	};

	/// @brief 端部样式
	/// @details 该类型与PIT::EndType大致对应
	struct End {
		/// @brief 端部类型
		EndType type;
		/// @brief 偏移
		/// @details 这个可以用来延长或缩短钢筋, 单位为mm
		double offset;
		/// @brief 旋转角, 单位degree
		/// @details 这个是弯锚的法向的旋转角, 0度为贴着墙
		double angle;
		/// @brief 不同端部类型对应的参数
		EndTypeData data;
	};

	/// @brief 钢筋方向
	enum class RebarDirection : int
	{
		/// @brief 横向钢筋
		Horizontal = 0,
		/// @brief 竖向钢筋
		Vertical = 1,
	};

	/// @brief 钢筋位置
	/// @details 与g_listRebarPose对应
	enum class RebarPosition : int
	{
		/// @brief 正面
		Front = 0,
		/// @brief 中间
		Middle = 1,
		/// @brief 背面
		Back = 2,
	};

	// @brief 廊道配筋相关设置, 对应PIT::Concrete
	struct GallerySettings {
		/// @brief 正面保护层厚度
		double  front_cover;
		/// @brief 背面保护层厚度
		double  back_cover;	
		/// @brief 侧面保护层厚度
		double  side_cover; 
		/// @brief 钢筋层数
		int layer_count; 
		/// @brief 是否需要规避孔洞
		bool handle_hole; 
		/// @brief 需要忽略的孔洞大小
		double hole_size; 

		/// @brief 每一组钢筋的配置，对应PIT::ConcreteRebar
		struct Layer {
			/// @brief 层序号
			int layer_index;
			/// @brief 钢筋方向
			RebarDirection direction;
			/// @brief 钢筋尺寸
			char size_str[512]; 
			/// @brief 钢筋型号/等级
			/// @details 对应grade
			int type;
			/// @brief 钢筋间距, MM
			double spacing;
			/*/// @brief 起点偏移
			double start_offset;
			/// @brief 终点偏移
			double end_offset;
			/// @brief 钢筋层间距
			double layer_space;*/
			/// @brief 钢筋位置
			RebarPosition position;
			//	钢筋颜色
			//int rebarColor;
			//  钢筋线形
			int rebarLineStyle;
			//  钢筋线宽
			int rebarWeight;
			/// @brief 起始端部样式
			End starting_end_type;
			/// @brief 结束端部样式
			End ending_end_type;

			/// @brief  组合钢筋size_key与钢筋等级（type）
        	/// @details  例如尺寸为14，等级、类型为0，则为14A，等级为ABCD
			/// @return 
			std::string get_full_size_key() const;

			/// @brief  获取钢筋直径
			/// @details 该方法会将钢筋size_key与钢筋等级(type)组合查询
			/// 		 与直接通过size_key获得的直径有差别
			///          如果model_ref为空的话，则取ACTIVEMODEL
			/// @param layer 
			/// @return 
			double get_rebar_diameter(DgnModelRefP model_ref = nullptr) const;

			/// @brief 获得钢筋弯曲半径
			/// @param model_ref 
			/// @return 
			double get_pin_radius(DgnModelRefP model_ref = nullptr) const;

			/// @brief 获得钢筋某个弯曲端部的锚入长度
			/// @param end 端部
			/// @param model_ref 
			/// @return 
			double get_bend_length(const End &end, DgnModelRefP model_ref = nullptr) const;

			/// @brief 获得当前钢筋层的Lae参数(锚固长度）
			/// @return 返回UOR为单位的Lae长度，如果获取失败，返回值小于0
			double get_lae()const;

			/// @brief 获得当前钢筋层的L0参数（搭接长度）
			/// @return 返回UOR为单位的L0长度，如果获取失败，返回值小于0
			double get_la0()const;
		};

		/// @brief 钢筋层信息
		/// @details layer_count是该数组的实际长度
		Layer layers[32];

	public:
		/// @brief 获取默认设置
		static GallerySettings from_default();

		/// @brief 读取存储在项目中的设置
		/// @param settings 读到的设置
		/// @details 如果该函数返回false, settings中数据无效
		/// @return 成功读到为true
		static bool from_project(GallerySettings &settings);

		/// @brief 读取存储在元素中的设置
		/// @param id 读取的元素
		/// @param settings 写入的数据
		/// @param model model
		/// @details 如果该函数返回false, settings中数据无效
		/// @return 元素中有数据返回true
		static bool from_element(ElementId id, GallerySettings &settings, DgnModelRefP model = ACTIVEMODEL);

		/// @brief 读取存储在元素中的设置
		/// @brief 对传id函数的一个封装
		/// @param eeh 
		/// @param settings 
		/// @return 
		static bool from_element(EditElementHandleCR eeh, GallerySettings &settings);

		/// @brief 保存设置到项目中
		/// @return 成功返回true
		bool save_to_project() const ;

		/// @brief 保存设置到元素中
		/// @return 成功返回true
		bool save_to_element(ElementId id, DgnModelRefP model = ACTIVEMODEL) const ;

		/// @brief 保存设置到元素中
		/// @return 成功返回true
		bool save_to_element(EditElementHandleR eeh) const ;
		

		/// @brief 追加添加新的层
		/// @param count 要添加的层数
		void add_default_layer(int count);

		/// @brief 删除后面的层
		/// @param count 要删除的层数
		void remove_layer(int count);

		/// @brief 转换到PIT::Concrete
		PIT::Concrete to_pit_concrete() const;

		/// @brief  转换到PIT::ConcreteRebar
		std::vector<PIT::ConcreteRebar> to_vec_concrete_rebar() const;

		/// @brief 转换到PIT::EndType
		std::vector<PIT::EndType> to_vec_end_type() const;

	};
}

