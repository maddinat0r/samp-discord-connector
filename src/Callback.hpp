#pragma once

#include <string>
#include <functional>
#include <list>
#include <unordered_set>
#include <algorithm>
#include <memory>
#include <vector>
#include <cstring>

namespace pawn_cb
{
	namespace _internal
	{
		class Manager
		{
		public: // singleton
			static Manager &Get()
			{
				static Manager instance;
				return instance;
			}

		private: //constructor / destructor
			Manager() = default;
			~Manager() = default;

		private: //variables
			std::unordered_set<AMX *> _amxInstances;

		public: //functions
			inline void AddAmx(AMX *amx)
			{
				_amxInstances.insert(amx);
			}
			inline void RemoveAmx(AMX *amx)
			{
				_amxInstances.erase(amx);
			}

			inline bool IsValidAmx(AMX *amx) const
			{
				return _amxInstances.count(amx) == 1;
			}
			inline void ForEach(std::function<bool(AMX *)> func)
			{
				for (auto *a : _amxInstances)
					if (!func(a))
						return;
			}
		};
	}

	inline void AddAmx(AMX *amx)
	{
		_internal::Manager::Get().AddAmx(amx);
	}
	inline void RemoveAmx(AMX *amx)
	{
		_internal::Manager::Get().RemoveAmx(amx);
	}

	class Error
	{
		friend class Callback;
	public:
		enum class Type
		{
			NONE,
			INVALID_AMX,
			INVALID_PARAMETERS,
			INVALID_PARAM_COUNT,
			INVALID_FORMAT_SPECIFIER,
			EMPTY_NAME,
			NAME_TOO_LONG,
			NOT_FOUND,
			EXPECTED_ARRAY_SIZE,
			INVALID_ARRAY_SIZE,
			NO_ARRAY_SIZE,
		};

	public:
		Error() : _type(Type::NONE) { }
		~Error() = default;

	private:
		Type _type;

		void set(Type type)
		{
			_type = type;
		}

		void reset()
		{
			set(Type::NONE);
		}

	public:
		Type get() const
		{
			return _type;
		}
		operator Type() const
		{
			return get();
		}
		operator bool() const
		{
			return get() != Type::NONE;
		}
	};

	using Callback_t = std::shared_ptr<class Callback>;
	class Callback
	{
	private:
		class Param
		{
			friend class ::pawn_cb::Callback;
		public:
			enum class Type
			{
				INVALID,
				CELL,
				STRING,
				ARRAY,
				REFERENCE,
			};

		private:
			Type _type;

			union
			{
				cell Cell;
				std::string String;
				std::vector<cell> Array;
				cell *Reference;
			};

		public:
			Param() : _type(Type::INVALID) { }
			template<typename T, typename std::enable_if<
				std::is_arithmetic<T>::value, cell>::type = 0>
				Param(T value) :
				_type(Type::CELL),
				Cell(static_cast<cell>(std::is_floating_point<T>::value ? amx_ftoc(value) : value))
			{ }
			Param(const char *value) :
				_type(Type::STRING)
			{
				new(&String) std::string(value);
			}
			Param(std::string &&value) :
				_type(Type::STRING)
			{
				new(&String) std::string(value);
			}

			template<typename T, typename std::enable_if<
				std::is_arithmetic<T>::value, cell>::type = 0>
				Param(std::vector<T> const &value) :
				_type(Type::ARRAY)
			{
				new(&Array) std::vector<cell>(value.size());

				std::transform(value.begin(), value.end(), std::back_inserter(Array),
					[](T const &v) {
					return std::is_floating_point<T>::value ? amx_ftoc(v) : static_cast<cell>(v);
				});
			}
			Param(cell *array, size_t size) :
				_type(Type::ARRAY)
			{
				new(&Array) std::vector<cell>(array[0], array[size]);
			}
			Param(cell *ref) :
				_type(Type::REFERENCE),
				Reference(ref)
			{ }

			Param(Param const &rhs) = delete;
			Param &operator=(Param const &rhs) = delete;
			Param(Param &&rhs) : _type(rhs._type)
			{
				switch (_type)
				{
				case Type::CELL:
					Cell = rhs.Cell;
					break;
				case Type::STRING:
					new(&String) std::string(std::move(rhs.String));
					break;
				case Type::ARRAY:
					new(&Array) std::vector<cell>(std::move(rhs.Array));
					break;
				case Type::REFERENCE:
					Reference = rhs.Reference;
					break;
				case Type::INVALID:
					// initialize nothing
					break;
				}
			}
			Param &operator=(Param &&rhs)
			{
				_type = rhs._type;
				switch (_type)
				{
				case Type::CELL:
					Cell = rhs.Cell;
					break;
				case Type::STRING:
					new(&String) std::string(std::move(rhs.String));
					break;
				case Type::ARRAY:
					new(&Array) std::vector<cell>(std::move(rhs.Array));
					break;
				case Type::REFERENCE:
					Reference = rhs.Reference;
					break;
				case Type::INVALID:
					// initialize nothing
					break;
				}
				return *this;
			}

			void Push(AMX *amx, cell &push_addr)
			{
				cell tmp;
				switch (_type)
				{
				case Type::CELL:
					amx_Push(amx, Cell);
					break;
				case Type::STRING:
					amx_PushString(amx, &tmp, nullptr, String.c_str(), 0, 0);
					if (push_addr < 0)
						push_addr = tmp;
					break;
				case Type::ARRAY:
					amx_PushArray(amx, &tmp, nullptr, Array.data(), Array.size());
					if (push_addr < 0)
						push_addr = tmp;
					break;
				case Type::REFERENCE:
					amx_PushAddress(amx, Reference);
					break;
				case Type::INVALID:
					// initialize nothing
					break;
				}
			}

		public:
			~Param()
			{
				if (_type == Type::STRING)
					String.~basic_string();
				else if (_type == Type::ARRAY)
					Array.~vector();
			}
		};

	private: //type definitions
		using ParamList_t = std::list<Param>;

	private: //constructor / destructor
		Callback(AMX *amx, int cb_idx, ParamList_t &&params) :
			_amx(amx),
			_amx_index(cb_idx),
			_params(std::move(params))
		{ }

	public:
		~Callback() = default;

	private: //variables
		AMX *_amx = nullptr;
		int _amx_index = -1;

		ParamList_t _params;

	public: //functions
		bool Execute(cell &ret_val)
		{
			cell amx_address = -1;
			for (auto &p : _params)
				p.Push(_amx, amx_address);

			ret_val = 0;
			int error = amx_Exec(_amx, &ret_val, _amx_index);

			if (amx_address >= 0)
				amx_Release(_amx, amx_address);
			return error == AMX_ERR_NONE;
		}
		bool Execute()
		{
			cell return_value;
			bool error = !Execute(return_value);
			if (error)
			{
				return false;
			}
			return static_cast<bool>(return_value);
		}

	private:
		template<typename T, typename... Tv>
		static void AddArgs(ParamList_t &list, T value, Tv&&... tail)
		{
			list.emplace_front(value);
			AddArgs(list, std::forward<Tv>(tail)...);
		}

		static void AddArgs(ParamList_t &list)
		{
			(void)list; // unused
		}

		static bool DoBasicInit(Error &error, AMX *amx, const char *name, int &cb_idx)
		{
			error.reset();

			if (amx == nullptr)
			{
				error.set(Error::Type::INVALID_AMX);
				return false;
			}

			if (name == nullptr || strlen(name) == 0)
			{
				error.set(Error::Type::EMPTY_NAME);
				return false;
			}

			if (strlen(name) > 31)
			{
				error.set(Error::Type::NAME_TOO_LONG);
				return false;
			}

			cb_idx = -1;
			if (amx_FindPublic(amx, name, &cb_idx) != AMX_ERR_NONE || cb_idx < 0)
			{
				error.set(Error::Type::NOT_FOUND);
				return false;
			}

			return true;
		}

	public: //factory functions
		template<typename... Tv>
		static Callback_t Prepare(Error &error, AMX *amx, const char *name, Tv&&... params)
		{
			int cb_idx;
			if (!DoBasicInit(error, amx, name, cb_idx))
				return nullptr;

			ParamList_t param_list;
			AddArgs(param_list, std::forward<Tv>(params)...);

			return Callback_t(new Callback(amx, cb_idx, std::move(param_list)));
		}

		static Callback_t Prepare(AMX *amx, const char *name, const char *format,
			cell *params, cell param_offset, Error &error)
		{
			int cb_idx;
			if (!DoBasicInit(error, amx, name, cb_idx))
				return nullptr;

			const size_t num_params = (format == nullptr) ? 0 : strlen(format);
			if ((params[0] / sizeof(cell) - (param_offset - 1)) != num_params)
			{
				error.set(Error::Type::INVALID_PARAM_COUNT);
				return nullptr;
			}

			ParamList_t param_list;
			if (num_params != 0)
			{
				cell param_idx = 0;
				cell *address_ptr = nullptr;
				cell *array_addr_ptr = nullptr;

				do
				{
					if (array_addr_ptr != nullptr && (*format) != 'd' && (*format) != 'i')
					{
						error.set(Error::Type::EXPECTED_ARRAY_SIZE);
						return nullptr;
					}

					switch (*format)
					{
					case 'd': //decimal
					case 'i': //integer
					{
						amx_GetAddr(amx, params[param_offset + param_idx], &address_ptr);
						cell value = *address_ptr;
						if (array_addr_ptr != nullptr)
						{
							if (value <= 0)
							{
								error.set(Error::Type::INVALID_ARRAY_SIZE);
								return nullptr;
							}

							param_list.emplace_front(array_addr_ptr, value);
							array_addr_ptr = nullptr;
						}

						param_list.emplace_front(value);
					}
					break;
					case 'f': //float
					case 'b': //bool
						amx_GetAddr(amx, params[param_offset + param_idx], &address_ptr);
						param_list.emplace_front(*address_ptr);
						break;
					case 's': //string
						param_list.emplace_front(
							amx_GetCppString(amx, params[param_offset + param_idx]));
						break;
					case 'a': //array
						amx_GetAddr(amx, params[param_offset + param_idx], &array_addr_ptr);
						break;
					case 'r': //reference
						amx_GetAddr(amx, params[param_offset + param_idx], &address_ptr);
						param_list.emplace_front(address_ptr);
						break;
					default:
						error.set(Error::Type::INVALID_FORMAT_SPECIFIER);
						return nullptr;
					}
					param_idx++;
				} while (*(++format) != '\0');

				if (array_addr_ptr != nullptr)
				{
					error.set(Error::Type::NO_ARRAY_SIZE);
					return nullptr;
				}
			}

			return Callback_t(new Callback(amx, cb_idx, std::move(param_list)));
		}

		template<typename... Tv>
		static bool Call(Error &error, AMX *amx, cell &ret_val, const char *name, Tv&&... params)
		{
			int cb_idx;
			if (!DoBasicInit(error, amx, name, cb_idx))
				return false;

			ParamList_t param_list;
			AddArgs(param_list, std::forward<Tv>(params)...);

			Callback cb(amx, cb_idx, std::move(param_list));
			return cb.Execute(ret_val);
		}

		template<typename... Tv>
		static bool Call(Error &error, AMX *amx, const char *name, Tv&&... params)
		{
			cell unused;
			return Call(error, amx, unused, name, std::forward<Tv>(params)...);
		}

		template<typename... Tv>
		static bool CallFirst(Error &error, cell &ret_val, const char *name, Tv&&... params)
		{
			bool success = false;
			_internal::Manager::Get().ForEach([&](AMX *amx)
			{
				bool res = Call(error, amx, ret_val, name, std::forward<Tv>(params)...);
				if (error == Error::Type::NOT_FOUND)
					return true; // continue / skip

				if (error || !res)
					return false; // break

				// callback was successfully called, stop lookup and break out of loop
				success = true;
				return false; // break
			});
			return success;
		}

		template<typename... Tv>
		static bool CallFirst(Error &error, const char *name, Tv&&... params)
		{
			cell unused;
			return CallFirst(error, unused, name, std::forward<Tv>(params)...);
		}
	};
}
