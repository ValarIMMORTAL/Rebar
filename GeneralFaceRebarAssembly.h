#pragma once

#include <Rebar/RebarModel.h>
#include <vector>
#include <functional>
#include "GallerySettings.h"

namespace Gallery
{

    /// @brief ͨ�������
    /// @details �ֲ����
    class GeneralFaceRebarAssembly : public RebarAssembly
    {
    public:
        /// @brief ���ļ��㷽ʽ
        enum class ArrayMode : int
        {
            /// @brief ���������з�ʽ
            /// @details ���ַ�ʽ�������������У�������ܸպ����¾ͻ�����һ������ͼ
            ///          |-------------|
            ///          |---|---|---|--
            Normal = 0,
            /// @brief �����������з�ʽ��ͬ����������ڲ��ܸպ�����ʱ������󲹳�һ���ֽ�
            /// @details ��ͼ
            ///          |-------------|
            ///          |---|---|---|-|
            NormalWithExtra = 1,
            /// @brief ����Ӧ�������
            /// @details �������з�ʽ������µļ�࣬ʹ����Ŀ�귶Χ���ܸպ�����
            ///          ������㷽ʽΪ��
            ///          ���s1 = 300
            ///          �ܳ���l = 1000
            ///          |-------------|
            ///          |---|---|---|--
            ///          �������n1 = floor(l/s1) = 3
            ///          �������n2 = n1 + 1 = 4
            ///          �µļ��s2 = floor(l/n2) = 250
            ///          ���Ĳ��ý��������250Ϊ��࣬����5��,����ͼ
            ///          |-----------|
            ///          |--|--|--|--|
            Fit = 2,
        };

        /// @brief ����һ��ֽ�����Ҫ�Ĳ���
        struct LayerRebarData {
            /// @brief �ò�ֽ�Ĳ���
            /// @details �ڲ���ʹ����layer_index��size_str��type��spacing��start_end_type��end_end_type�ֶ� 
            const GallerySettings::Layer *layer;

            /// @brief �ֽ�����ƣ���ȡTEXT_MAIN_REBAR��TEXT_TWIN_REBAR��
            const wchar_t *level_name;

            /// @brief ���з�ʽ
            ArrayMode array_mode;

            /// @brief ���ɸֽ�ʱ���ڵ�ƽ��
            EditElementHandle *face;

            /// @brief ǽ�ķ�����Ϊ��ê�Ļ�׼
            DVec3d wall_normal;

            /// @brief �ֽ�򣬻ᱻͶӰ����ƽ��
            DVec3d rebar_dir;

            /// @brief ���汣������, ��λMM
            double side_cover;

            /// @brief ��ƫ��
            /// @details �����ֵ��Ϊ0�������һ���ֽ���
            ///          ����ֽ�����ɷ���ƫ�ƣ��÷�����rebar_dir��wall_normal�Ĳ�ˣ�
            ///          ���һ���ķ����෴
            ///          �������Ա�֤���ֽ�ʱ�����ƶ�������������A
            ///          ��λΪUOR
            ///          һ��ȡֵΪn���ĸֽ�ֱ��
            double collision_offset;
        };

    public:
        /// @brief ��concrete�л��֮ǰ�����ĸ���
        /// @param concrete_id ������id,����ͨ��FetchConcrete���
        /// @param model_ref ���������ڵ�model_ref, һ����ACTIVEMODEL
        /// @return ����ָ��ǰ���͵�ָ�룬���û�ҵ�������nullptr
        static GeneralFaceRebarAssembly *from_concrete(ElementId concrete_id, DgnModelRefP model_ref);

        /// @brief ����һ���µ�rebar assembly
        /// @param model_ref ���������ڵ�model_ref������˵�ǵ�ǰRebarAssembly���ڵ�model_ref, һ����ACTIVEMODEL��
        /// @param element_id Ԫ�ص�id (���ǻ�������Id)
        /// @param element_model_ref Ԫ�����ڵ�model
        /// @return 
        static GeneralFaceRebarAssembly *create(DgnModelRefP model_ref, ElementId element_id, DgnModelRefP element_model_ref);

        // ��Ҫ�����(��create��from_concrete)
        GeneralFaceRebarAssembly(ElementId element_id, DgnModelRefP model_ref);
        GeneralFaceRebarAssembly(): RebarAssembly() {}
        virtual ~GeneralFaceRebarAssembly() = default;

        /// @brief ����һ������
        void begin_generate(void){ 
            this->NewRebarAssembly(this->GetModelRef());
            this->m_set_tag_count = 1;
            this->m_set_tags.Clear(true);
        }

        /// @brief ��������
        void end_generate(void) {
            this->AddRebarSets(this->m_set_tags, this->GetModelRef());
            this->Save(this->GetModelRef());
        }

        /// @brief ����ָ���Ĳ�������һ��ֽ�
        /// @details ����ʱ��minɨ�赽max�������з�ʽ���ɸֽ� 
        bool generate_layer(const LayerRebarData &layer_rebar_data);

        /// @brief ɾ��֮ǰ���ɵĸֽ�
        /// @return ɾ���ɹ�����true
        void remove_rebar(void);

        /// @brief ����˫��ʱ�Ļص�����
        /// @param callback 
        void on_double_click(const std::function<bool(GeneralFaceRebarAssembly &)> &callback)
        {
            this->m_double_click_callback = callback;
        }

        virtual bool OnDoubleClick() override;

    public:

    private:
        int m_set_tag_count;
        RebarSetTagArray m_set_tags;
        std::function<bool(GeneralFaceRebarAssembly &)> m_double_click_callback;

        /// @brief ���ݲ��������ɶ���ֽ���
        /// @param new_spacing ʵ�ʵļ��
        /// @return
        static std::vector<RebarCurve> gen_rebar_curves(const LayerRebarData &layer_rebar_data, double diameter, double &new_spacing, RebarEndTypes &end_types);

    public:
        virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
        virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 1; }
        virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"General Faces Rebar Assembly"; }
        virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"General Faces Rebar Assembly"; }
        
        // ��������У������޷�ͨ��RebarAssemblies���ҵ�ǰ��
        BE_DECLARE_VMS(GeneralFaceRebarAssembly, RebarAssembly)
    };
}

