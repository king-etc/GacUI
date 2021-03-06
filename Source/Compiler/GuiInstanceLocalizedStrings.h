/***********************************************************************
Vczh Library++ 3.0
Developer: Zihan Chen(vczh)
GacUI Reflection: Shared Script

Interfaces:
***********************************************************************/

#ifndef VCZH_PRESENTATION_REFLECTION_GUIINSTANCELOCALIZEDSTRINGS
#define VCZH_PRESENTATION_REFLECTION_GUIINSTANCELOCALIZEDSTRINGS

#include "../Resources/GuiResource.h"
#include "../../Import/VlppWorkflowCompiler.h"

namespace vl
{
	namespace presentation
	{
		class GuiInstanceLocalizedStrings : public Object, public Description<GuiInstanceLocalizedStrings>
		{
		public:
			class StringItem : public Object
			{
			public:
				WString									name;
				WString									text;
				GuiResourceTextPos						textPosition;
			};

			class Strings : public Object
			{
				using StringItemMap = collections::Dictionary<WString, Ptr<StringItem>>;
			public:
				collections::List<WString>				locales;
				StringItemMap							items;
				GuiResourceTextPos						tagPosition;

				WString									GetLocalesName();
			};

			WString										className;
			WString										defaultLocale;
			collections::List<Ptr<Strings>>				strings;
			GuiResourceTextPos							tagPosition;

			static Ptr<GuiInstanceLocalizedStrings>		LoadFromXml(Ptr<GuiResourceItem> resource, Ptr<parsing::xml::XmlDocument> xml, GuiResourceError::List& errors);
			Ptr<parsing::xml::XmlElement>				SaveToXml();
			Ptr<workflow::WfModule>						Compile(GuiResourcePrecompileContext& precompileContext, const WString& moduleName, GuiResourceError::List& errors);
		};
	}
}

#endif