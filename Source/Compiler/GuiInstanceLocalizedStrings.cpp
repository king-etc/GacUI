#include "GuiInstanceLocalizedStrings.h"
#include "GuiInstanceLoader.h"

namespace vl
{
	namespace presentation
	{
		using namespace collections;
		using namespace parsing;
		using namespace parsing::xml;
		using namespace workflow;
		using namespace workflow::analyzer;
		using namespace reflection::description;

/***********************************************************************
GuiInstanceLocalizedStrings
***********************************************************************/

		WString GuiInstanceLocalizedStrings::Strings::GetLocalesName()
		{
			return From(locales).Aggregate(WString(L""), [](const WString& a, const WString& b)
			{
				return a == L"" ? b : a + L";" + b;
			});
		}

		Ptr<GuiInstanceLocalizedStrings> GuiInstanceLocalizedStrings::LoadFromXml(Ptr<GuiResourceItem> resource, Ptr<parsing::xml::XmlDocument> xml, GuiResourceError::List& errors)
		{
			auto ls = MakePtr<GuiInstanceLocalizedStrings>();

			if (xml->rootElement->name.value!=L"LocalizedStrings")
			{
				errors.Add(GuiResourceError({ { resource },xml->rootElement->codeRange.start }, L"Precompile: The root element of localized strings should be \"LocalizedStrings\"."));
				return nullptr;
			}
			ls->tagPosition = { {resource},xml->rootElement->name.codeRange.start };

			auto attClassName = XmlGetAttribute(xml->rootElement, L"ref.Class");
			if (!attClassName)
			{
				errors.Add(GuiResourceError({ { resource },xml->rootElement->codeRange.start }, L"Precompile: Missing attribute \"ref.Class\" in \"LocalizedStrings\"."));
			}
			else
			{
				ls->className = attClassName->value.value;
			}

			auto attDefaultLocale = XmlGetAttribute(xml->rootElement, L"DefaultLocale");
			if (!attDefaultLocale)
			{
				errors.Add(GuiResourceError({ { resource },xml->rootElement->codeRange.start }, L"Precompile: Missing attribute \"DefaultLocale\" in \"LocalizedStrings\"."));
			}
			else
			{
				ls->defaultLocale = attDefaultLocale->value.value;
			}

			if (!attClassName || !attDefaultLocale)
			{
				return nullptr;
			}

			SortedList<WString> existingLocales;
			FOREACH(Ptr<XmlElement>, xmlStrings, XmlGetElements(xml->rootElement))
			{
				if (xmlStrings->name.value != L"Strings")
				{
					errors.Add(GuiResourceError({ { resource },xmlStrings->codeRange.start }, L"Precompile: Unknown element \"" + xmlStrings->name.value + L"\", it should be \"Strings\"."));
					continue;
				}

				auto attLocales = XmlGetAttribute(xmlStrings, L"Locales");
				if (!attLocales)
				{
					errors.Add(GuiResourceError({ { resource },xmlStrings->codeRange.start }, L"Precompile: Missing attribute \"Locales\" in \"Strings\"."));
				}
				else
				{
					auto lss = MakePtr<GuiInstanceLocalizedStrings::Strings>();
					ls->strings.Add(lss);
					lss->tagPosition = { { resource },xmlStrings->name.codeRange.start };
					SplitBySemicolon(attLocales->value.value, lss->locales);

					FOREACH(WString, locale, lss->locales)
					{
						if (!existingLocales.Contains(locale))
						{
							existingLocales.Add(locale);
						}
						else
						{
							errors.Add(GuiResourceError({ { resource },attLocales->codeRange.start }, L"Precompile: Locale \"" + locale + L"\" already exists."));
						}
					}

					FOREACH(Ptr<XmlElement>, xmlString, XmlGetElements(xmlStrings))
					{
						if (xmlString->name.value != L"String")
						{
							errors.Add(GuiResourceError({ { resource },xmlString->codeRange.start }, L"Precompile: Unknown element \"" + xmlString->name.value + L"\", it should be \"String\"."));
							continue;
						}

						auto attName = XmlGetAttribute(xmlString, L"Name");
						auto attText = XmlGetAttribute(xmlString, L"Text");

						if (!attName)
						{
							errors.Add(GuiResourceError({ { resource },xmlString->codeRange.start }, L"Precompile: Missing attribute \"Name\" in \"String\"."));
						}
						if (!attText)
						{
							errors.Add(GuiResourceError({ { resource },xmlString->codeRange.start }, L"Precompile: Missing attribute \"Text\" in \"String\"."));
						}

						if (attName && attText)
						{
							if (lss->items.Keys().Contains(attName->value.value))
							{
								errors.Add(GuiResourceError({ { resource },xmlString->codeRange.start }, L"Precompile: String \"" + attName->value.value + L"\" already exists."));
							}
							else
							{
								auto item = MakePtr<GuiInstanceLocalizedStrings::StringItem>();
								item->name = attName->value.value;
								item->text = attText->value.value;
								item->textPosition = { {resource},attText->value.codeRange.start };
								item->textPosition.column += 1;
								lss->items.Add(item->name, item);
							}
						}
					}
				}
			}

			if (!existingLocales.Contains(ls->defaultLocale))
			{
				errors.Add(GuiResourceError({ { resource },xml->rootElement->codeRange.start }, L"Precompile: Strings for the default locale \"" + ls->defaultLocale + L"\" is not defined."));
			}

			return ls;
		}

		Ptr<parsing::xml::XmlElement> GuiInstanceLocalizedStrings::SaveToXml()
		{
			auto xml = MakePtr<XmlElement>();
			xml->name.value = L"LocalizedStrings";
			{
				auto att = MakePtr<XmlAttribute>();
				att->name.value = L"ref.Class";
				att->value.value = className;
				xml->attributes.Add(att);
			}
			{
				auto att = MakePtr<XmlAttribute>();
				att->name.value = L"DefaultLocale";
				att->value.value = defaultLocale;
				xml->attributes.Add(att);
			}

			FOREACH(Ptr<GuiInstanceLocalizedStrings::Strings>, lss, strings)
			{
				auto xmlStrings = MakePtr<XmlElement>();
				xml->subNodes.Add(xmlStrings);
				xmlStrings->name.value = L"Strings";
				{
					auto att = MakePtr<XmlAttribute>();
					att->name.value = L"Strings";
					att->value.value = lss->GetLocalesName();
					xmlStrings->attributes.Add(att);
				}

				FOREACH(Ptr<GuiInstanceLocalizedStrings::StringItem>, lssi, lss->items.Values())
				{
					auto xmlString = MakePtr<XmlElement>();
					xmlStrings->subNodes.Add(xmlString);
					{
						auto att = MakePtr<XmlAttribute>();
						att->name.value = L"Name";
						att->value.value = lssi->name;
						xmlString->attributes.Add(att);
					}
					{
						auto att = MakePtr<XmlAttribute>();
						att->name.value = L"Text";
						att->value.value = lssi->text;
						xmlString->attributes.Add(att);
					}
				}
			}

			return xml;
		}

		using ParameterPair = Pair<Ptr<ITypeInfo>, WString>;
		using ParameterList = List<ParameterPair>;
		using ParameterGroup = Group<WString, ParameterPair>;

		static bool ParseLocalizedText(
			const WString& text,
			ParameterList& parameters,
			List<vint>& positions,
			List<WString>& texts,
			GuiResourceTextPos pos,
			GuiResourceError::List& errors
			)
		{
			const wchar_t* reading = text.Buffer();
			const wchar_t* textPosCounter = reading;
			ParsingTextPos formatPos(0, 0);

			auto addError = [&](const WString& message)
			{
				auto errorPos = pos;
				errorPos.row += formatPos.row;
				errorPos.column = (formatPos.row == 0 ? errorPos.column : 0) + formatPos.column;
				errors.Add({ errorPos,message });
			};

			while (*reading)
			{
				const wchar_t* begin = wcsstr(reading, L"$(");
				if (begin)
				{
					texts.Add(WString(reading, vint(begin - reading)));
				}
				else
				{
					break;
				}

				const wchar_t* end = wcsstr(begin, L")");
				if (!end)
				{
					addError(L"Precompile: Does not find matched close bracket.");
					return false;
				}

				while (textPosCounter++ < begin + 2)
				{
					switch (textPosCounter[-1])
					{
					case '\n':
						formatPos.row++;
						formatPos.column = 0;
						break;
					default:
						formatPos.column++;
						break;
					}
				}

				if (end - begin == 3 && wcsncmp(begin, L"$($)", 4) == 0)
				{
					if (texts.Count() > 0)
					{
						texts[texts.Count() - 1] += L"$";
					}
					else
					{
						texts.Add(L"$");
					}
				}
				else
				{
					const wchar_t* number = begin + 2;
					const wchar_t* numberEnd = number;
					while (L'0' <= *numberEnd && *numberEnd < L'9')
					{
						numberEnd++;
					}

					if (number == numberEnd)
					{
						addError(L"Precompile: Unexpected character, the correct format is $(index) or $(index:function).");
						return false;
					}

					Ptr<ITypeInfo> type;
					WString function;
					if (*numberEnd == L':')
					{
						if (end - numberEnd > 1)
						{
							function = WString(numberEnd + 1, (vint)(end - numberEnd - 1));
							if (function == L"ShortDate" || function == L"LongDate" || function == L"YearMonthDate" || function == L"ShortTime" || function == L"LongTime")
							{
								type = TypeInfoRetriver<DateTime>::CreateTypeInfo();
							}
							else if (function == L"Number" || function == L"Currency")
							{
								type = TypeInfoRetriver<WString>::CreateTypeInfo();
							}
							else
							{
								addError(L"Precompile: Unknown formatting function name \"" + function + L"\".");
								return false;
							}
						}
						else
						{
							addError(L"Precompile: Unexpected character, the correct format is $(index) or $(index:function).");
							return false;
						}
					}
					else if (numberEnd != end)
					{
						addError(L"Precompile: Unexpected character, the correct format is $(index) or $(index:function).");
						return false;
					}

					if (!type)
					{
						type = TypeInfoRetriver<WString>::CreateTypeInfo();
					}
					parameters.Add({ type,function });
					positions.Add(wtoi(WString(number, (vint)(numberEnd - number))));
				}
				reading = end;
			}

			if (*reading || texts.Count() == 0)
			{
				texts.Add(reading);
			}

			FOREACH_INDEXER(vint, i, index, From(positions).OrderBy([](vint a, vint b) {return a - b; }))
			{
				if (i != index)
				{
					errors.Add({ pos,L"Precompile: Missing parameter \"" + itow(index) + L"\"." });
					return false;
				}
			}
			return true;
		}

		Ptr<workflow::WfModule> GuiInstanceLocalizedStrings::Compile(GuiResourcePrecompileContext& precompileContext, const WString& moduleName, GuiResourceError::List& errors)
		{
			auto defaultStrings = From(strings)
				.Where([=](Ptr<Strings> strings)
				{
					return strings->locales.Contains(defaultLocale);
				})
				.First();

			vint errorCount = errors.Count();
			FOREACH(Ptr<Strings>, lss, strings)
			{
				if (lss != defaultStrings)
				{
					auto localesName = lss->GetLocalesName();

					auto missing = From(defaultStrings->items.Keys())
						.Except(lss->items.Keys())
						.Aggregate(WString(L""), [](const WString& a, const WString& b)
						{
							return a == L"" ? b : a + L", " + b;
						});
					
					auto extra = From(lss->items.Keys())
						.Except(defaultStrings->items.Keys())
						.Aggregate(WString(L""), [](const WString& a, const WString& b)
						{
							return a == L"" ? b : a + L", " + b;
						});

					if (missing != L"")
					{
						errors.Add({ lss->tagPosition,L"Precompile: Missing strings for locale \"" + localesName + L"\": " + missing + L"." });
					}

					if (extra != L"")
					{
						errors.Add({ lss->tagPosition,L"Precompile: Unnecessary strings for locale \"" + localesName + L"\": " + extra + L"." });
					}
				}
			}
			if (errors.Count() != errorCount)
			{
				return nullptr;
			}

			ParameterGroup defaultGroup;
			FOREACH(Ptr<StringItem>, lssi, defaultStrings->items.Values())
			{
				ParameterList parameters;
				List<vint> positions;
				List<WString> texts;

				if (ParseLocalizedText(lssi->text, parameters, positions, texts, lssi->textPosition, errors))
				{
					FOREACH(vint, index, positions)
					{
						defaultGroup.Add(lssi->name, parameters[index]);
					}
				}
			}
			if (errors.Count() != errorCount)
			{
				return nullptr;
			}

			auto defaultLocalesName = defaultStrings->GetLocalesName();
			FOREACH(Ptr<Strings>, lss, strings)
			{
				if (lss != defaultStrings)
				{
					auto localesName = lss->GetLocalesName();

					FOREACH(Ptr<StringItem>, lssi, lss->items.Values())
					{
						ParameterList parameters;
						List<vint> positions;
						List<WString> texts;

						if (ParseLocalizedText(lssi->text, parameters, positions, texts, lssi->textPosition, errors))
						{
							auto& defaultParameters = defaultGroup[lssi->name];
							if (defaultParameters.Count() != parameters.Count())
							{
								errors.Add({ lss->tagPosition,L"String \"" + lssi->name + L"\" in locales \"" + defaultLocalesName + L"\" and \"" + localesName + L"\" have different numbers of parameters." });
							}
							else
							{
								for (vint i = 0; i < parameters.Count(); i++)
								{
									auto defaultParameter = defaultParameters[i];
									auto parameter = parameters[positions[i]];

									if (defaultParameter.key->GetTypeDescriptor()->GetTypeName() != parameter.key->GetTypeDescriptor()->GetTypeName())
									{
										errors.Add({ lss->tagPosition,L"Parameter \"" + itow(i) + L"\" in String \"" + lssi->name + L"\" in locales \"" + defaultLocalesName + L"\" and \"" + localesName + L"\" are in different types \"" + defaultParameter.key->GetTypeFriendlyName() + L"\" and \"" + parameter.key->GetTypeFriendlyName() + L"\"." });
									}
								}
							}
						}
					}
				}
			}
			if (errors.Count() != errorCount)
			{
				return nullptr;
			}

			auto module = MakePtr<WfModule>();
			module->name.value = moduleName;
			return module;
		}
	}
}