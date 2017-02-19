

		for(UINT b=0;b<srcBands;b++) {
			LPBYTE pDst=ppDst[b]+dstOff;
			LPBYTE pSrc=ppSrc[b];

			for(int y=0;y<srcLenY;y+=2) {
				int y1=y*NTI_BLKWIDTH;
				int y2=y1;
				if(y<srcLenY-1) y2+=NTI_BLKWIDTH;

				LPBYTE p=pDst;	
				for(int x=0;x<srcLenX;x+=2) {
					int x2=(x<srcLenX-1)?(x+1):x;
					*p++=( pSrc[y1+x] + pSrc[y1+x2] +
							pSrc[y2+x] + pSrc[y2+x2] + 2) >> 2;
				}
				pDst+=NTI_BLKWIDTH;
			}
		}

		//=============================================================

		BYTE b[4],g[4],r[4];
		BYTE bt=m_crTransColor&0xff,gt=(m_crTransColor>>8)&0xff,rt=(m_crTransColor>>16)&0xff,et=(m_crTransColor>>24);

		LPBYTE pSrcB=ppSrc[0],pSrcG=ppSrc[1],pSrcR=ppSrc[2];
		LPBYTE pDstB=ppDst[0]+dstOff,pDstG=ppDst[1]+dstOff,pDstR=ppDst[2]+dstOff;

		for(int y=0;y<srcLenY;y+=2) {
			int y1=y*NTI_BLKWIDTH;
			int y2=y1;
			if(y<srcLenY-1) y2+=NTI_BLKWIDTH;

			for(int x=0;x<srcLenX;x+=2) {
				int x2=(x<srcLenX-1)?(x+1):x;

				b[0]=pSrcB[y1+x];
				b[1]=pSrcB[y1+x2];
				b[2]=pSrcB[y2+x];
				b[3]=pSrcB[y2+x2];
				g[0]=pSrcG[y1+x];
				g[1]=pSrcG[y1+x2];
				g[2]=pSrcG[y2+x];
				g[3]=pSrcG[y2+x2];
				r[0]=pSrcR[y1+x];
				r[1]=pSrcR[y1+x2];
				r[2]=pSrcR[y2+x];
				r[3]=pSrcR[y2+x2];

				int bs=0,gs=0,rs=0,nDif=0; //colors different
				for(int i=0;i<4;i++) {
					if(((bt ^ b[i]) | (gt ^ g[i]) | (rt ^ r[i]))<et) continue;
					nDif++;
					bs+=b[i];
					gs+=g[i];
					rs+=r[i];
				}
				if(nDif) {
					bs=(bs+nDif/2)/nDif;
					gs=(gs+nDif/2)/nDif;
					rs=(rs+nDif/2)/nDif;
				}
				else {
					bs=bt;
					gs=gt;
					rs=rt;
				}
				*pDstG++=bs;
				*pDstG++=gs;
				*pDstR++=rs;
			}
			pDstB+=NTI_BLKWIDTH;
			pDstG+=NTI_BLKWIDTH;
			pDstR+=NTI_BLKWIDTH;
		}
