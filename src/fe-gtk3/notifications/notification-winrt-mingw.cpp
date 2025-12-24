/* PChat
 * Copyright (C) 2025 Zach Bacon
 * MinGW-compatible Windows 8+ Toast notification backend using WinRT COM APIs
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <versionhelpers.h>
#include <roapi.h>
#include <windows.ui.notifications.h>
#include <windows.data.xml.dom.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;

static ComPtr<IToastNotifier> g_notifier;
static bool g_initialized = false;

// Helper to convert UTF-8 to wide string
static HSTRING CreateHStringFromUTF8(const char* utf8_str)
{
	if (!utf8_str) return nullptr;
	
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, nullptr, 0);
	if (len <= 0) return nullptr;
	
	wchar_t* wstr = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wstr, len);
	
	HSTRING result;
	WindowsCreateString(wstr, len - 1, &result);
	delete[] wstr;
	
	return result;
}

extern "C"
{
	__declspec(dllexport) void
	notification_backend_show(const char* title, const char* text)
	{
		if (!g_initialized || !g_notifier)
			return;

		HRESULT hr;
		
		// Get ToastNotificationManager statics
		ComPtr<IToastNotificationManagerStatics> toastStatics;
		HSTRING className = nullptr;
		WindowsCreateString(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager,
		                    wcslen(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager),
		                    &className);
		
		hr = RoGetActivationFactory(className, IID_PPV_ARGS(&toastStatics));
		WindowsDeleteString(className);
		if (FAILED(hr)) return;

		// Get template XML for ToastText02
		ComPtr<IXmlDocument> toastXml;
		hr = toastStatics->GetTemplateContent(ToastTemplateType_ToastText02, &toastXml);
		if (FAILED(hr)) return;

		// Get text elements
		ComPtr<IXmlNodeList> textNodes;
		HSTRING textTag = nullptr;
		WindowsCreateString(L"text", 4, &textTag);
		hr = toastXml->GetElementsByTagName(textTag, &textNodes);
		WindowsDeleteString(textTag);
		if (FAILED(hr)) return;

		UINT32 length = 0;
		textNodes->get_Length(&length);

		// Set title text (first node)
		if (length >= 1)
		{
			ComPtr<IXmlNode> titleNode;
			hr = textNodes->Item(0, &titleNode);
			if (SUCCEEDED(hr))
			{
				HSTRING titleHStr = CreateHStringFromUTF8(title);
				if (titleHStr)
				{
					ComPtr<IXmlText> textContent;
					hr = toastXml->CreateTextNode(titleHStr, &textContent);
					WindowsDeleteString(titleHStr);
					
					if (SUCCEEDED(hr))
					{
						ComPtr<IXmlNode> textNode;
						textContent.As(&textNode);
						ComPtr<IXmlNode> appendedChild;
						titleNode->AppendChild(textNode.Get(), &appendedChild);
					}
				}
			}
		}

		// Set message text (second node)
		if (length >= 2)
		{
			ComPtr<IXmlNode> messageNode;
			hr = textNodes->Item(1, &messageNode);
			if (SUCCEEDED(hr))
			{
				HSTRING messageHStr = CreateHStringFromUTF8(text);
				if (messageHStr)
				{
					ComPtr<IXmlText> textContent;
					hr = toastXml->CreateTextNode(messageHStr, &textContent);
					WindowsDeleteString(messageHStr);
					
					if (SUCCEEDED(hr))
					{
						ComPtr<IXmlNode> textNode;
						textContent.As(&textNode);
						ComPtr<IXmlNode> appendedChild;
						messageNode->AppendChild(textNode.Get(), &appendedChild);
					}
				}
			}
		}

		// Create and show toast
		ComPtr<IToastNotificationFactory> toastFactory;
		HSTRING toastClassName = nullptr;
		WindowsCreateString(RuntimeClass_Windows_UI_Notifications_ToastNotification,
		                    wcslen(RuntimeClass_Windows_UI_Notifications_ToastNotification),
		                    &toastClassName);
		
		hr = RoGetActivationFactory(toastClassName, IID_PPV_ARGS(&toastFactory));
		WindowsDeleteString(toastClassName);
		if (FAILED(hr)) return;

		ComPtr<IToastNotification> toast;
		hr = toastFactory->CreateToastNotification(toastXml.Get(), &toast);
		if (FAILED(hr)) return;

		g_notifier->Show(toast.Get());
	}

	__declspec(dllexport) int
	notification_backend_init(const char** error)
	{
		if (g_initialized)
			return 1;

		HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);
		if (FAILED(hr))
		{
			*error = "Failed to initialize Windows Runtime";
			return 0;
		}

		// Get ToastNotificationManager
		ComPtr<IToastNotificationManagerStatics> toastStatics;
		HSTRING className = nullptr;
		WindowsCreateString(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager,
		                    wcslen(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager),
		                    &className);
		
		hr = RoGetActivationFactory(className, IID_PPV_ARGS(&toastStatics));
		WindowsDeleteString(className);
		
		if (FAILED(hr))
		{
			*error = "Failed to get ToastNotificationManager";
			RoUninitialize();
			return 0;
		}

		// Create notifier with app ID
		HSTRING appId = nullptr;
		WindowsCreateString(L"PChat.Desktop.Notify", 20, &appId);
		hr = toastStatics->CreateToastNotifierWithId(appId, &g_notifier);
		WindowsDeleteString(appId);
		
		if (FAILED(hr))
		{
			*error = "Failed to create toast notifier";
			RoUninitialize();
			return 0;
		}

		g_initialized = true;
		return 1;
	}

	__declspec(dllexport) void
	notification_backend_deinit(void)
	{
		if (!g_initialized)
			return;

		g_notifier.Reset();
		RoUninitialize();
		g_initialized = false;
	}

	__declspec(dllexport) int
	notification_backend_supported(void)
	{
		// Toast notifications require Windows 8+
		return IsWindows8OrGreater();
	}
}
