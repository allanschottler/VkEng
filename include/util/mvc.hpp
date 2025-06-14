#pragma once
#include <optional>
#include <functional>
#include <concepts>
#include <memory>
#include <iostream>

using namespace std;
using namespace std::placeholders;

template<class Msg> using send_f = function<void(Msg)>;
template<class Model, class Msg> using view_f = function<void(Model&, send_f<Msg>)>;
template<class Model, class Msg> using update_f = function<void(Model&, Msg)>;

template<
	class Model,
	class Msg,
	invocable<Model&, send_f<Msg>> View = view_f<Model, Msg>,
	invocable<Model&, Msg> Update = update_f<Model, Msg>
>
struct mvc {
	using msg_t = Msg;
	using model_t = Model;
	using view_t = View;
	using update_t = Update;
		
	unique_ptr<model_t> model;
	view_t view;
	update_t update;

	send_f<Msg> send;
		
	explicit mvc(
		View pview,
		Update pupdate)
		: model(make_unique<Model>())
		, view(pview)
		, update(pupdate)
		, send(bind(&mvc::receive, this, _1))
	{}

	explicit mvc(
		Model* pmodel,
		View pview,
		Update pupdate)
		: model(pmodel)
		, view(pview)
		, update(pupdate)
		, send(bind(&mvc::receive, this, _1))
	{}

	template <typename U = Model> 
		requires is_lvalue_reference_v<U>
	explicit mvc(
		Model pmodel,
		View pview,
		Update pupdate)
		: model(make_unique<Model>(&pmodel))
		, view(pview)
		, update(pupdate)
		, send(bind(&mvc::receive, this, _1))
	{}
	
    template <typename U = Model> 
		requires (!is_lvalue_reference_v<U>)
	explicit mvc(
		Model pmodel,
		View pview,
		Update pupdate)
		: model(make_unique<Model>(move(pmodel)))
		, view(pview)
		, update(pupdate)
		, send(bind(&mvc::receive, this, _1))
	{}

	void draw() { view(*model, send); }

	void receive(Msg msg) {
		update(*model, msg);
		draw();
	}

};
