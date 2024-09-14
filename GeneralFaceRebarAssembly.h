#pragma once

#include <Rebar/RebarModel.h>
#include <vector>
#include <functional>
#include "GallerySettings.h"

namespace Gallery
{

    /// @brief 通用面配筋
    /// @details 分层配筋
    class GeneralFaceRebarAssembly : public RebarAssembly
    {
    public:
        /// @brief 间距的计算方式
        enum class ArrayMode : int
        {
            /// @brief 正常的排列方式
            /// @details 这种方式就是正常的排列，如果不能刚好排下就会少排一根，如图
            ///          |-------------|
            ///          |---|---|---|--
            Normal = 0,
            /// @brief 与正常的排列方式相同，但是这个在不能刚好排下时会在最后补充一根钢筋
            /// @details 如图
            ///          |-------------|
            ///          |---|---|---|-|
            NormalWithExtra = 1,
            /// @brief 自适应间距排列
            /// @details 这种排列方式会计算新的间距，使得在目标范围内能刚好排下
            ///          具体计算方式为：
            ///          间距s1 = 300
            ///          总长度l = 1000
            ///          |-------------|
            ///          |---|---|---|--
            ///          间隔数量n1 = floor(l/s1) = 3
            ///          间隔数量n2 = n1 + 1 = 4
            ///          新的间距s2 = floor(l/n2) = 250
            ///          最后的布置结果就是以250为间距，布置5根,如下图
            ///          |-----------|
            ///          |--|--|--|--|
            Fit = 2,
        };

        /// @brief 创建一层钢筋所需要的参数
        struct LayerRebarData {
            /// @brief 该层钢筋的参数
            /// @details 内部仅使用了layer_index、size_str、type、spacing、start_end_type、end_end_type字段 
            const GallerySettings::Layer *layer;

            /// @brief 钢筋层名称，可取TEXT_MAIN_REBAR、TEXT_TWIN_REBAR等
            const wchar_t *level_name;

            /// @brief 排列方式
            ArrayMode array_mode;

            /// @brief 生成钢筋时所在的平面
            EditElementHandle *face;

            /// @brief 墙的法向，作为弯锚的基准
            DVec3d wall_normal;

            /// @brief 钢筋方向，会被投影到该平面
            DVec3d rebar_dir;

            /// @brief 侧面保护层厚度, 单位MM
            double side_cover;

            /// @brief 错开偏移
            /// @details 如果该值不为0，除最后一根钢筋外
            ///          都会钢筋的生成方向偏移（该方向是rebar_dir和wall_normal的叉乘）
            ///          最后一根的方向相反
            ///          这样可以保证错开钢筋时不会移动到保护层外面A
            ///          单位为UOR
            ///          一般取值为n倍的钢筋直径
            double collision_offset;
        };

    public:
        /// @brief 从concrete中获得之前创建的该类
        /// @param concrete_id 混凝土id,可以通过FetchConcrete获得
        /// @param model_ref 混凝土所在的model_ref, 一般是ACTIVEMODEL
        /// @return 返回指向当前类型的指针，如果没找到，返回nullptr
        static GeneralFaceRebarAssembly *from_concrete(ElementId concrete_id, DgnModelRefP model_ref);

        /// @brief 创建一个新的rebar assembly
        /// @param model_ref 混凝土所在的model_ref（或者说是当前RebarAssembly所在的model_ref, 一般是ACTIVEMODEL）
        /// @param element_id 元素的id (不是混凝土的Id)
        /// @param element_model_ref 元素所在的model
        /// @return 
        static GeneralFaceRebarAssembly *create(DgnModelRefP model_ref, ElementId element_id, DgnModelRefP element_model_ref);

        // 不要用这个(用create或from_concrete)
        GeneralFaceRebarAssembly(ElementId element_id, DgnModelRefP model_ref);
        GeneralFaceRebarAssembly(): RebarAssembly() {}
        virtual ~GeneralFaceRebarAssembly() = default;

        /// @brief 启动一次生成
        void begin_generate(void){ 
            this->NewRebarAssembly(this->GetModelRef());
            this->m_set_tag_count = 1;
            this->m_set_tags.Clear(true);
        }

        /// @brief 结束生成
        void end_generate(void) {
            this->AddRebarSets(this->m_set_tags, this->GetModelRef());
            this->Save(this->GetModelRef());
        }

        /// @brief 根据指定的参数生成一层钢筋
        /// @details 生成时从min扫描到max，按排列方式生成钢筋 
        bool generate_layer(const LayerRebarData &layer_rebar_data);

        /// @brief 删除之前生成的钢筋
        /// @return 删除成功返回true
        void remove_rebar(void);

        /// @brief 设置双击时的回调函数
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

        /// @brief 根据参数，生成多根钢筋线
        /// @param new_spacing 实际的间距
        /// @return
        static std::vector<RebarCurve> gen_rebar_curves(const LayerRebarData &layer_rebar_data, double diameter, double &new_spacing, RebarEndTypes &end_types);

    public:
        virtual BentleyStatus GetPresentation(EditElementHandleR eeh, ViewContextR viewContext) override { return SUCCESS; }
        virtual int         GetPolymorphic() const override { return REX_Type::kLastRebarElement + kRebarAssembly + 1; }
        virtual WString     GetDescriptionEx(ElementHandleCR eh) const override { return L"General Faces Rebar Assembly"; }
        virtual WString     GetPathDescriptionEx(ElementHandleCR eh) const override { return L"General Faces Rebar Assembly"; }
        
        // 这个必须有，否则无法通过RebarAssemblies查找当前类
        BE_DECLARE_VMS(GeneralFaceRebarAssembly, RebarAssembly)
    };
}

