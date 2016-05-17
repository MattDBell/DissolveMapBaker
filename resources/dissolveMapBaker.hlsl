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

	// Assume a completely black last frame (otherwise it'll get marked as black
	// on the first frame)
	int toWrite = 256;

	// Traverse backwards, if black write the "timestamp" that way the last black
	// occurance will be saved in the image
	for (int i = elements - 1; i >= 0; --i)
	{
		int val = frames[uint3(DTid.xy, i)];
		if (val == 0)
		{
			toWrite = i * 256 / elements;
		}
	}

	output[DTid.xy] = toWrite;
}
