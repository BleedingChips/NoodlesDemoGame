#include <cassert>
#include <imgui.h>

import std;
import Potato;
import Dumpling;
import Noodles;
import MapoToufu;

struct A {};
struct B {};

using namespace MapoToufu;


auto Fun = [](float color, float speed, float time) -> float
{
	return std::fmod(color + speed * time, 2.0f);
};

struct DemoWeight : public Dumpling::IGWidget
{
	virtual void Draw(Dumpling::PassRenderer& render) override
	{
		ImGui::ShowDemoWindow();
	}

protected:

	virtual void AddIGWidgetRef() const {}
	virtual void SubIGWidgetRef() const {}
}demo;

auto dying = AutoSystemNodeStatic(
	[]() {
		Potato::Log::Log<u8"Dying">(Potato::Log::Level::Log, "DyingSystemNode");
	}
);

struct Pipeline
{
	PassIndex clean_up_pass_index;
	PassIndex imgui_pass_index;
	Dumpling::PassSequencer sequence;
	Potato::IR::StructLayoutObject::Ptr parameter;
	Dumpling::Color color;
};

int main()
{

	Dumpling::Color color = {1.0f, 1.0f, 1.0f, 1.0f};
	Dumpling::Color new_color{ color };
	GameContext context;
	RendererSubModule::Config config;
	config.priority.layer = 1;
	config.priority.primary_priority = 100;
	auto renderer_module = MapoToufu::RendererSubModule::Create(config);
	context.RegisterModule(*renderer_module);
	auto instance = context.CreatInstance();

	auto entity = instance->CreateEntity();

	auto dying_index = instance->PrepareSystemNode(
		&dying
	);

	instance->LoadDyingSystemNode(
		dying_index
	);

	auto thread_id = std::this_thread::get_id();
	//renderer_module->AddFormComponent(*instance, *entity);

	SystemNode::Parameter par2;
	par2.name = u8"UpdatePipeline";

	auto index = instance->PrepareSystemNode(
		CreateAutoSystemNode([&](Context& context, AutoComponentQuery<Form, IGHud, Pipeline> c_query, AutoSingletonQuery<PassDistributor> singleton) {
			
			auto history = context.GetEntityHistory();

			for (auto& ite : history)
			{
				if (c_query.RequireMatchAll(ite.add))
				{
					volatile int o = 0;
				}

				if (c_query.RequireMatchSingle<IGHud>(ite.add))
				{
					volatile int o = 0;
				}
			}

			auto distributor = singleton.Query(context)->GetPointer<0>();
			if (distributor == nullptr)
				return;

			c_query.Foreach(context, [=](decltype(c_query)::Data& data) -> bool {
				
				for (auto [form, hud, pipeline] : data)
				{
					if (pipeline->sequence.GetRequireCount() == 0)
					{
						auto require_pass = std::array<std::u8string_view, 2>{ 
							MapoToufu::CleanViewTargetPass::GetPassName(),
							MapoToufu::IGHUDPass::GetPassName() 
						};
						auto parameter = distributor->CreatePassRequest(
							u8"normal_pipeline",
							require_pass,
							pipeline->sequence
						);
						if (parameter.has_value())
						{
							pipeline->parameter = *parameter;
						}
					}

					auto k = sizeof(MapoToufu::CleanViewTargetPass::Property);
					auto p = sizeof(MapoToufu::IGHUDPass::Property);

					if (pipeline->parameter)
					{
						pipeline->color.R = Fun(pipeline->color.R, 0.2f, context.GetInstance().GetDeltaTime().count());
						pipeline->color.G = Fun(pipeline->color.G, 0.3f, context.GetInstance().GetDeltaTime().count());
						pipeline->color.B = Fun(pipeline->color.B, 0.4f, context.GetInstance().GetDeltaTime().count());
						auto new_color = pipeline->color;
						new_color.R = std::abs(new_color.R - 1.0f);
						new_color.G = std::abs(new_color.G - 1.0f);
						new_color.B = std::abs(new_color.B - 1.0f);

						auto p1 = pipeline->parameter->MemberAs<MapoToufu::CleanViewTargetPass::Property>(0);
						auto p2 = pipeline->parameter->MemberAs<MapoToufu::IGHUDPass::Property>(1);

						p1->clean_color[0] = new_color;
						p1->target.Clear();
						p1->target.AddRenderTarget(*form->GetRenderTarget());
						p2->SetParameter(*form->GetRenderTarget(), *hud);

						distributor->SendRequest(pipeline->sequence);
					}
					
				}
				
				
				return true;
				});
		})
	);

	instance->LoadSystemNode(SystemCategory::Tick, index, par2);
	SystemNode::Parameter par;
	par.name = u8"FuckYou";
	par.acceptable_mask = *renderer_module->GetCreateWindowThreadMask();
	
	auto index22 = instance->LoadOnceSystemNode(
		CreateAutoSystemNode(
			[](Context& context, AutoSingletonQuery<FrameRenderer const> singleton) {
				auto render = singleton.Query(context)->GetPointer<0>();
				if (render != nullptr)
				{
					auto entity = context.GetInstance().CreateEntity();
					auto form = render->CreateForm();

					auto im_hud = render->CreateIGHUD(form, &demo);
					context.GetInstance().AddEntityComponent(*entity, std::move(form));
					context.GetInstance().AddEntityComponent(*entity, std::move(im_hud));
					Pipeline pipeline;
					context.GetInstance().AddEntityComponent(*entity, std::move(pipeline));
				}
			}
		),
		par
	);

	context.Launch(*instance);
	context.RunMainThreadAndWait();
	renderer_module->Destory(context);
}