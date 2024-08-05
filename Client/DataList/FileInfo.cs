using System;
using ProtoBuf;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace Client
{
    [ProtoContract]
    public class FileInfo
    {
        [ProtoMember(1)]
        public string fileType { get; set; }
        [ProtoMember(2)]
        public string fileName { get; set; }
        [ProtoMember(3)]
        public string fileSize { get; set; }
        [ProtoMember(4)]
        public string fileChangeTime { get; set; }
        [ProtoMember(5)]
        public string absolutePath { get; set; }
        [ProtoMember(6)]
        public bool isExpand { get; set; }
        [ProtoMember(7)]
        public ObservableCollection<FileInfo> subFileInfoList { get; set; }

        public FileInfo()
        {
            absolutePath = null;
            isExpand = false;
            subFileInfoList = new ObservableCollection<FileInfo>();
        }

        // 获取子文件信息列表
        public ObservableCollection<FileInfo> GetSubFileInfoList(string targetPath)
        {
            string[] twoPart = targetPath.Split(new[] { '\\' }, 2, StringSplitOptions.RemoveEmptyEntries);
            string firstFileName = twoPart[0];

            // 子文件不存在，则路径不存在
            FileInfo existFileInfo = null;
            foreach (FileInfo subFileInfo in subFileInfoList)
            {
                if (subFileInfo.fileName.ToLower() == firstFileName.ToLower() && subFileInfo.fileType == "📁")
                {
                    existFileInfo = subFileInfo;
                    break;
                }
            }
            if (existFileInfo == null)
            {
                return null;
            }

            // 没有下一条路径
            if (twoPart.Length == 1)
            {
                existFileInfo.isExpand = true;
                return existFileInfo.subFileInfoList;
            }

            // 还有下一条路径
            return existFileInfo.GetSubFileInfoList(twoPart[1]);
        }

        // 获取文件信息
        public FileInfo GetFileInfo(string targetPath)
        {
            string[] twoPart = targetPath.Split(new[] { '\\' }, 2, StringSplitOptions.RemoveEmptyEntries);
            string firstFileName = twoPart[0];

            // 文件信息不存在，则创建文件信息
            FileInfo existFileInfo = null;
            foreach (FileInfo subFileInfo in subFileInfoList)
            {
                if (subFileInfo.fileName.ToLower() == firstFileName.ToLower() && subFileInfo.fileType == "📁")
                {
                    subFileInfo.isExpand = true;
                    existFileInfo = subFileInfo;
                    break;
                }
            }
            if (existFileInfo == null)
            {
                existFileInfo = new FileInfo() { fileType = "📁", fileName = firstFileName, fileSize = "0", fileChangeTime = "Unknown", absolutePath = absolutePath + "\\" + firstFileName, isExpand = true };
                subFileInfoList.Add(existFileInfo);
            }

            // 没有下一条路径
            if (twoPart.Length == 1)
            {
                return existFileInfo;
            }

            // 还有下一条路径
            return existFileInfo.GetFileInfo(twoPart[1]);
        }

        // 更新子文件信息列表
        public void UpdateSubFileInfoList(List<FileInfo> newSubFileInfoList)
        {
            // 去除原本存在，现在不存在的文件信息
            List<FileInfo> notExistFileInfoList = new List<FileInfo>();
            foreach (FileInfo subFileInfo in subFileInfoList)
            {
                bool exist = false;
                foreach (FileInfo newSubFileInfo in newSubFileInfoList)
                {
                    if (newSubFileInfo.fileName.ToLower() == subFileInfo.fileName.ToLower() && newSubFileInfo.fileType == subFileInfo.fileType)
                    {
                        exist = true;
                        break;
                    }
                }
                if (!exist)
                {
                    notExistFileInfoList.Add(subFileInfo);
                }
            }
            foreach (FileInfo notExistFileInfo in notExistFileInfoList)
            {
                subFileInfoList.Remove(notExistFileInfo);
            }

            // 添加原本不存在，现在存在的文件信息
            foreach (FileInfo newSubFileInfo in newSubFileInfoList)
            {
                bool exist = false;
                foreach (FileInfo oldSubFileInfo in subFileInfoList)
                {
                    if (oldSubFileInfo.fileName.ToLower() == newSubFileInfo.fileName.ToLower() && oldSubFileInfo.fileType == newSubFileInfo.fileType)
                    {
                        exist = true;
                        break;
                    }
                }
                if (!exist)
                {
                    newSubFileInfo.absolutePath = absolutePath + "\\" + newSubFileInfo.fileName;
                    subFileInfoList.Add(newSubFileInfo);
                }
            }
        }
    }
}