Texture2DArray<int> frames : register(t0);
RWTexture2D<uint> output : register(u0);

SamplerState sam
{
	Filter = MIN_MAP_MIP_POINT;
	AddressU = Wrap;
	AddressV = Wrap;
};

[numthreads(1,1,1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	uint width;
	uint height;
	uint elements;
	uint numLevels;

	frames.GetDimensions(0, width, height, elements, numLevels);
	float2 uv = float2((float)DTid.x / width, (float)DTid.y / height);

	//output[DTid.xy] = uv.x * 256; //DTid.x * 256 / width;

	for (int i = elements - 1; i >= 0; --i)
	{

		int val = frames[uint3(DTid.xy, i)];// frames.Gather(sam, float3(uv.x, uv.y, 0.5f), int2(0, 0)).r;
		//output[DTid.xy] = val * 256;
		if (val == 0)
		{
			output[DTid.xy] = i * 256 / elements;
		}
	}

}