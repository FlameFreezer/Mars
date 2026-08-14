//Fields subobject with more than one field
template<size_t size, class T, class... Types>
struct Fields {
	T field[size];
	Fields<size, Types...> others;

	template<size_t fieldNum>
	auto& get() noexcept {
		return others.template get<fieldNum - 1>();
	}

	template<size_t fieldNum>
	const auto& get() const noexcept {
		return others.template get<fieldNum - 1>();
	}

	template<>
	auto& get<0>() noexcept {
		return field;
	}

	template<>
	const auto& get<0>() const noexcept {
		return field;
	}
};

//Fields subobject with only one field
template<size_t size, class T>
struct Fields<size, T> {
	T field[size];
	template<size_t fieldNum>
	auto& get() = delete;

	template<size_t fieldNum>
	const auto& get() const = delete;

	template<>
	auto& get<0>() noexcept {
		return field;
	}

	template<>
	const auto& get<0>() const noexcept {
		return field;
	}
};

template<size_t size, class... Types>
class MultiArray {
	Fields<size, Types...> mFields;

public:
	template<size_t fieldNum>
	auto& get() noexcept {
		return mFields.template get<fieldNum>();
	}

	template<size_t fieldNum>
	const auto& get() const noexcept {
		return mFields.template get<fieldNum>();
	}

	size_t capacity() const noexcept {
		return size;
	}
};
