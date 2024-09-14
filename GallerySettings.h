#pragma once

#include "CommonFile.h"

namespace Gallery
{
	/// @brief �˲�������
	/// @details ��g_listEndType��Ӧ
	enum class EndType : int
	{
		/// @brief ��
		None = 0,
		/// @brief ����
		Bend = 1,
		/// @brief ����
		Hook = 2,
		/// @brief ����
		PolygonalLine = 3,
		/// @brief 90���乳
		Hook90 = 4,
		/// @brief 135���乳
		Hook135 = 5,
		/// @brief 180���乳
		Hook180 = 6,
		/// @brief ֱê
		Straight = 7,
		/// @brief �û��Զ���
		Custom = 8,
		/// @brief �Զ�����ê
		/// @details ����������Ƕ�������
		CustomBend = 9,
	};

	/// @brief ֱê����
	struct EndTypeDataStraight {
		/// @brief ê�볤��
		double length;
	};

	/// @brief ��ͬ�������乳�Ĳ���
	struct EndTypeDataHookDegree {
		/// @brief �����뾶
		double radius;
		/// @brief Ԥ������
		/// @details Ӧ����������ĳ���
		double reserver_length;
	};

	/// @brief �Զ�����ê
	struct EndTypeCustomBend{
		/// @brief �����Ƕ� ��λdeg
		/// @details ����Ƕ��Ǹֽ��ⲿ�ļнǣ�����ͼ��ʾ
		///      \ 
		///       \ <--here
		///--------...........
		/// �ֽ�     �ӳ���
		double angle;
		/// @brief �����뾶
		double radius;
		/// @brief ��������
		double length;
	};


	/// @brief ��ͬ�˲����͵Ĳ���
	/// @details �����¶���Ĳ������⣬��������û�в���
	union EndTypeData {
		/// @brief ����Ϊֱêʱ�Ĳ���
		EndTypeDataStraight straight;
		/// @brief ����Ϊ���������乳ʱ�Ĳ���
		EndTypeDataHookDegree hook_degree;
		/// @brief �Զ�����ê
		EndTypeCustomBend custom_bend;
	};

	/// @brief �˲���ʽ
	/// @details ��������PIT::EndType���¶�Ӧ
	struct End {
		/// @brief �˲�����
		EndType type;
		/// @brief ƫ��
		/// @details ������������ӳ������̸ֽ�, ��λΪmm
		double offset;
		/// @brief ��ת��, ��λdegree
		/// @details �������ê�ķ������ת��, 0��Ϊ����ǽ
		double angle;
		/// @brief ��ͬ�˲����Ͷ�Ӧ�Ĳ���
		EndTypeData data;
	};

	/// @brief �ֽ��
	enum class RebarDirection : int
	{
		/// @brief ����ֽ�
		Horizontal = 0,
		/// @brief ����ֽ�
		Vertical = 1,
	};

	/// @brief �ֽ�λ��
	/// @details ��g_listRebarPose��Ӧ
	enum class RebarPosition : int
	{
		/// @brief ����
		Front = 0,
		/// @brief �м�
		Middle = 1,
		/// @brief ����
		Back = 2,
	};

	// @brief �ȵ�����������, ��ӦPIT::Concrete
	struct GallerySettings {
		/// @brief ���汣������
		double  front_cover;
		/// @brief ���汣������
		double  back_cover;	
		/// @brief ���汣������
		double  side_cover; 
		/// @brief �ֽ����
		int layer_count; 
		/// @brief �Ƿ���Ҫ��ܿ׶�
		bool handle_hole; 
		/// @brief ��Ҫ���ԵĿ׶���С
		double hole_size; 

		/// @brief ÿһ��ֽ�����ã���ӦPIT::ConcreteRebar
		struct Layer {
			/// @brief �����
			int layer_index;
			/// @brief �ֽ��
			RebarDirection direction;
			/// @brief �ֽ�ߴ�
			char size_str[512]; 
			/// @brief �ֽ��ͺ�/�ȼ�
			/// @details ��Ӧgrade
			int type;
			/// @brief �ֽ���, MM
			double spacing;
			/*/// @brief ���ƫ��
			double start_offset;
			/// @brief �յ�ƫ��
			double end_offset;
			/// @brief �ֽ����
			double layer_space;*/
			/// @brief �ֽ�λ��
			RebarPosition position;
			//	�ֽ���ɫ
			//int rebarColor;
			//  �ֽ�����
			int rebarLineStyle;
			//  �ֽ��߿�
			int rebarWeight;
			/// @brief ��ʼ�˲���ʽ
			End starting_end_type;
			/// @brief �����˲���ʽ
			End ending_end_type;

			/// @brief  ��ϸֽ�size_key��ֽ�ȼ���type��
        	/// @details  ����ߴ�Ϊ14���ȼ�������Ϊ0����Ϊ14A���ȼ�ΪABCD
			/// @return 
			std::string get_full_size_key() const;

			/// @brief  ��ȡ�ֽ�ֱ��
			/// @details �÷����Ὣ�ֽ�size_key��ֽ�ȼ�(type)��ϲ�ѯ
			/// 		 ��ֱ��ͨ��size_key��õ�ֱ���в��
			///          ���model_refΪ�յĻ�����ȡACTIVEMODEL
			/// @param layer 
			/// @return 
			double get_rebar_diameter(DgnModelRefP model_ref = nullptr) const;

			/// @brief ��øֽ������뾶
			/// @param model_ref 
			/// @return 
			double get_pin_radius(DgnModelRefP model_ref = nullptr) const;

			/// @brief ��øֽ�ĳ�������˲���ê�볤��
			/// @param end �˲�
			/// @param model_ref 
			/// @return 
			double get_bend_length(const End &end, DgnModelRefP model_ref = nullptr) const;

			/// @brief ��õ�ǰ�ֽ���Lae����(ê�̳��ȣ�
			/// @return ����UORΪ��λ��Lae���ȣ������ȡʧ�ܣ�����ֵС��0
			double get_lae()const;

			/// @brief ��õ�ǰ�ֽ���L0��������ӳ��ȣ�
			/// @return ����UORΪ��λ��L0���ȣ������ȡʧ�ܣ�����ֵС��0
			double get_la0()const;
		};

		/// @brief �ֽ����Ϣ
		/// @details layer_count�Ǹ������ʵ�ʳ���
		Layer layers[32];

	public:
		/// @brief ��ȡĬ������
		static GallerySettings from_default();

		/// @brief ��ȡ�洢����Ŀ�е�����
		/// @param settings ����������
		/// @details ����ú�������false, settings��������Ч
		/// @return �ɹ�����Ϊtrue
		static bool from_project(GallerySettings &settings);

		/// @brief ��ȡ�洢��Ԫ���е�����
		/// @param id ��ȡ��Ԫ��
		/// @param settings д�������
		/// @param model model
		/// @details ����ú�������false, settings��������Ч
		/// @return Ԫ���������ݷ���true
		static bool from_element(ElementId id, GallerySettings &settings, DgnModelRefP model = ACTIVEMODEL);

		/// @brief ��ȡ�洢��Ԫ���е�����
		/// @brief �Դ�id������һ����װ
		/// @param eeh 
		/// @param settings 
		/// @return 
		static bool from_element(EditElementHandleCR eeh, GallerySettings &settings);

		/// @brief �������õ���Ŀ��
		/// @return �ɹ�����true
		bool save_to_project() const ;

		/// @brief �������õ�Ԫ����
		/// @return �ɹ�����true
		bool save_to_element(ElementId id, DgnModelRefP model = ACTIVEMODEL) const ;

		/// @brief �������õ�Ԫ����
		/// @return �ɹ�����true
		bool save_to_element(EditElementHandleR eeh) const ;
		

		/// @brief ׷������µĲ�
		/// @param count Ҫ��ӵĲ���
		void add_default_layer(int count);

		/// @brief ɾ������Ĳ�
		/// @param count Ҫɾ���Ĳ���
		void remove_layer(int count);

		/// @brief ת����PIT::Concrete
		PIT::Concrete to_pit_concrete() const;

		/// @brief  ת����PIT::ConcreteRebar
		std::vector<PIT::ConcreteRebar> to_vec_concrete_rebar() const;

		/// @brief ת����PIT::EndType
		std::vector<PIT::EndType> to_vec_end_type() const;

	};
}

