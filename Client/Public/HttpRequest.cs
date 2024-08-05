using System;
using System.Net;
using System.Text;
using System.Windows;
using System.Net.Http;
using Newtonsoft.Json;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace Client
{
    public class HttpRequest
    {
        public static string serverUrl;
        public static CookieContainer cookieContainer;
        private HttpClientHandler httpHandler;

        private string getParameter;
        private Dictionary<string, string> postParameter;

        public HttpRequest(string getParameter, Dictionary<string, string> postParameter)
        {
            this.getParameter = getParameter;
            this.postParameter = postParameter;
            httpHandler = new HttpClientHandler()
            {
                CookieContainer = cookieContainer,
                ServerCertificateCustomValidationCallback = delegate { return true; }
            };
        }

        // 通用 HTTP 请求
        public byte[] GeneralRequest()
        {
            HttpResponseMessage response;
            using (HttpClient httpClient = new HttpClient(httpHandler))
            {
                httpClient.Timeout = TimeSpan.FromSeconds(3);
                FormUrlEncodedContent content = new FormUrlEncodedContent(postParameter);
                try
                {
                    response = httpClient.PostAsync(serverUrl + getParameter, content).GetAwaiter().GetResult();
                }
                catch (Exception ex)
                {
                    MessageBox.Show("请求失败\n" + ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                    return Encoding.UTF8.GetBytes("HttpRequestError");
                }
            }
            if (response.IsSuccessStatusCode)
            {
                byte[] responseBody = response.Content.ReadAsByteArrayAsync().GetAwaiter().GetResult();
                if (responseBody.Length == 0)
                {
                    MessageBox.Show("响应为空 (空数据 / 权限不足)", "警告", MessageBoxButton.OK, MessageBoxImage.Warning);
                }
                return responseBody;
            }
            else
            {
                MessageBox.Show(((int)response.StatusCode).ToString() + " " + response.StatusCode.ToString(), "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                return Encoding.UTF8.GetBytes("HttpRequestError");
            }
        }

        // 获取列表 HTTP 请求
        public ObservableCollection<T> GetListRequest<T>()
        {
            HttpResponseMessage response;
            using (HttpClient httpClient = new HttpClient(httpHandler))
            {
                httpClient.Timeout = TimeSpan.FromSeconds(3);
                FormUrlEncodedContent content = new FormUrlEncodedContent(postParameter);
                try
                {
                    response = httpClient.PostAsync(serverUrl + getParameter, content).GetAwaiter().GetResult();
                }
                catch (Exception ex)
                {
                    MessageBox.Show("请求失败\n" + ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                    return null;
                }
            }
            if (response.IsSuccessStatusCode)
            {
                string json = response.Content.ReadAsStringAsync().GetAwaiter().GetResult();
                try
                {
                    return JsonConvert.DeserializeObject<ObservableCollection<T>>(json);
                }
                catch (Exception ex)
                {
                    MessageBox.Show("HTTP 响应数据无法转为 Json\n" + ex.Message, "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                    return null;
                }
            }
            else
            {
                MessageBox.Show(((int)response.StatusCode).ToString() + " " + response.StatusCode.ToString(), "错误", MessageBoxButton.OK, MessageBoxImage.Error);
                return null;
            }
        }
    }
}