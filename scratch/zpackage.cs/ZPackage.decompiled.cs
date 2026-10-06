using System;
using System.IO;
using System.Security.Cryptography;
using UnityEngine;

public class ZPackage
{
	private readonly MemoryStream m_stream = new MemoryStream();

	private BinaryWriter m_writer;

	private BinaryReader m_reader;

	public ZPackage()
	{
		m_writer = new BinaryWriter(m_stream);
		m_reader = new BinaryReader(m_stream);
	}

	public ZPackage(string base64String)
	{
		m_writer = new BinaryWriter(m_stream);
		m_reader = new BinaryReader(m_stream);
		if (!string.IsNullOrEmpty(base64String))
		{
			byte[] array = Convert.FromBase64String(base64String);
			m_stream.Write(array, 0, array.Length);
			m_stream.Position = 0L;
		}
	}

	public ZPackage(byte[] data)
	{
		m_writer = new BinaryWriter(m_stream);
		m_reader = new BinaryReader(m_stream);
		m_stream.Write(data, 0, data.Length);
		m_stream.Position = 0L;
	}

	public ZPackage(byte[] data, int dataSize)
	{
		m_writer = new BinaryWriter(m_stream);
		m_reader = new BinaryReader(m_stream);
		m_stream.Write(data, 0, dataSize);
		m_stream.Position = 0L;
	}

	public void SetReader(BinaryReader reader)
	{
		m_reader = reader;
	}

	public void SetWriter(BinaryWriter writer)
	{
		m_writer = writer;
	}

	public void Load(byte[] data)
	{
		Clear();
		m_stream.Write(data, 0, data.Length);
		m_stream.Position = 0L;
	}

	public void Write(ZPackage pkg)
	{
		byte[] array = pkg.GetArray();
		m_writer.Write(array.Length);
		m_writer.Write(array);
	}

	public void WriteCompressed(ZPackage pkg)
	{
		byte[] array = Utils.Compress(pkg.GetArray());
		m_writer.Write(array.Length);
		m_writer.Write(array);
	}

	public byte[] GetCompressed()
	{
		return Utils.Compress(GetArray());
	}

	public void Decompress()
	{
		byte[] array = Utils.Decompress(GetArray());
		m_stream.Position = 0L;
		m_stream.Write(array, 0, array.Length);
		m_stream.Position = 0L;
	}

	public void Write(byte[] array)
	{
		m_writer.Write(array.Length);
		m_writer.Write(array);
	}

	public void Write(byte data)
	{
		m_writer.Write(data);
	}

	public void Write(sbyte data)
	{
		m_writer.Write(data);
	}

	public void Write(char data)
	{
		m_writer.Write(data);
	}

	public void Write(bool data)
	{
		m_writer.Write(data);
	}

	public void Write(int data)
	{
		m_writer.Write(data);
	}

	public void Write(uint data)
	{
		m_writer.Write(data);
	}

	public void Write(short data)
	{
		m_writer.Write(data);
	}

	public void Write(ushort data)
	{
		m_writer.Write(data);
	}

	public void Write(long data)
	{
		m_writer.Write(data);
	}

	public void Write(ulong data)
	{
		m_writer.Write(data);
	}

	public void Write(float data)
	{
		m_writer.Write(data);
	}

	public void Write(double data)
	{
		m_writer.Write(data);
	}

	public void Write(string data)
	{
		m_writer.Write(data);
	}

	public void Write(ZDOID id)
	{
		m_writer.Write(id.UserID);
		m_writer.Write(id.ID);
	}

	public void Write(Vector3 v3)
	{
		m_writer.Write(v3.x);
		m_writer.Write(v3.y);
		m_writer.Write(v3.z);
	}

	public void WriteSmallRotation(Vector3 v3)
	{
		v3 *= 2f;
		uint num = (uint)v3.x;
		uint num2 = (uint)v3.y;
		uint num3 = (uint)v3.z;
		if ((num <= 1 || num >= 719) && (num3 <= 1 || num3 >= 719))
		{
			num2 |= 0x8000;
			m_writer.Write((short)num2);
			return;
		}
		uint num4 = num;
		num4 |= num2 << 10;
		num4 |= num3 << 20;
		m_writer.Write((ushort)(num4 >> 16));
		m_writer.Write((ushort)num4);
	}

	public void Write(Vector2i v2)
	{
		m_writer.Write(v2.x);
		m_writer.Write(v2.y);
	}

	public void Write(Vector2s v2)
	{
		m_writer.Write(v2.x);
		m_writer.Write(v2.y);
	}

	public void Write(Quaternion q)
	{
		m_writer.Write(q.x);
		m_writer.Write(q.y);
		m_writer.Write(q.z);
		m_writer.Write(q.w);
	}

	public void WriteNumItems(int numItems)
	{
		if (numItems < 128)
		{
			m_writer.Write((byte)numItems);
			return;
		}
		m_writer.Write((byte)((numItems >> 8) | 0x80));
		m_writer.Write((byte)numItems);
	}

	public ZDOID ReadZDOID()
	{
		return new ZDOID(m_reader.ReadInt64(), m_reader.ReadUInt32());
	}

	public bool ReadBool()
	{
		return m_reader.ReadBoolean();
	}

	public char ReadChar()
	{
		return m_reader.ReadChar();
	}

	public byte ReadByte()
	{
		return m_reader.ReadByte();
	}

	public int ReadNumItems()
	{
		int num = m_reader.ReadByte();
		if ((num & 0x80) != 0)
		{
			num = ((num & 0x7F) << 8) | m_reader.ReadByte();
		}
		return num;
	}

	public sbyte ReadSByte()
	{
		return m_reader.ReadSByte();
	}

	public short ReadShort()
	{
		return m_reader.ReadInt16();
	}

	public ushort ReadUShort()
	{
		return m_reader.ReadUInt16();
	}

	public int ReadInt()
	{
		return m_reader.ReadInt32();
	}

	public uint ReadUInt()
	{
		return m_reader.ReadUInt32();
	}

	public long ReadLong()
	{
		return m_reader.ReadInt64();
	}

	public ulong ReadULong()
	{
		return m_reader.ReadUInt64();
	}

	public float ReadSingle()
	{
		return m_reader.ReadSingle();
	}

	public double ReadDouble()
	{
		return m_reader.ReadDouble();
	}

	public string ReadString()
	{
		return m_reader.ReadString();
	}

	public Vector3 ReadVector3()
	{
		return new Vector3
		{
			x = m_reader.ReadSingle(),
			y = m_reader.ReadSingle(),
			z = m_reader.ReadSingle()
		};
	}

	public Vector3 ReadSmallRotation()
	{
		Vector3 vector = default;
		uint num = m_reader.ReadUInt16();
		if ((num & 0x8000) != 0)
		{
			vector.x = 0f;
			vector.z = 0f;
			vector.y = (float)(num & 0x7FFF) * 0.5f;
			return vector;
		}
		num = (num << 16) | m_reader.ReadUInt16();
		vector.x = num & 0x3FF;
		vector.y = (num >> 10) & 0x3FF;
		vector.z = (num >> 20) & 0x3FF;
		return vector * 0.5f;
	}

	public Vector2i ReadVector2i()
	{
		return new Vector2i
		{
			x = m_reader.ReadInt32(),
			y = m_reader.ReadInt32()
		};
	}

	public Vector2s ReadVector2s()
	{
		return new Vector2s
		{
			x = m_reader.ReadInt16(),
			y = m_reader.ReadInt16()
		};
	}

	public Quaternion ReadQuaternion()
	{
		return new Quaternion
		{
			x = m_reader.ReadSingle(),
			y = m_reader.ReadSingle(),
			z = m_reader.ReadSingle(),
			w = m_reader.ReadSingle()
		};
	}

	public ZPackage ReadCompressedPackage()
	{
		int count = m_reader.ReadInt32();
		return new ZPackage(Utils.Decompress(m_reader.ReadBytes(count)));
	}

	public ZPackage ReadPackage()
	{
		int count = m_reader.ReadInt32();
		return new ZPackage(m_reader.ReadBytes(count));
	}

	public void ReadPackage(ref ZPackage pkg)
	{
		int count = m_reader.ReadInt32();
		byte[] array = m_reader.ReadBytes(count);
		pkg.Clear();
		pkg.m_stream.Write(array, 0, array.Length);
		pkg.m_stream.Position = 0L;
	}

	public byte[] ReadByteArray()
	{
		int count = m_reader.ReadInt32();
		return m_reader.ReadBytes(count);
	}

	public byte[] ReadByteArray(int num)
	{
		return m_reader.ReadBytes(num);
	}

	public string GetBase64()
	{
		return Convert.ToBase64String(GetArray());
	}

	public byte[] GetArray()
	{
		m_writer.Flush();
		m_stream.Flush();
		return m_stream.ToArray();
	}

	public void SetPos(int pos)
	{
		m_stream.Position = pos;
	}

	public int GetPos()
	{
		return (int)m_stream.Position;
	}

	public int Size()
	{
		m_writer.Flush();
		m_stream.Flush();
		return (int)m_stream.Length;
	}

	public void Flush()
	{
		m_writer.Flush();
		m_stream.Flush();
	}

	public void Clear()
	{
		m_writer.Flush();
		m_stream.SetLength(0L);
		m_stream.Position = 0L;
	}

	public byte[] GenerateHash()
	{
		byte[] array = GetArray();
		return SHA512.Create().ComputeHash(array);
	}
}
