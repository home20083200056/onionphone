/*
 * ===================================================================
 *  TS 26.104
 *  REL-5 V5.4.0 2004-03
 *  REL-6 V6.1.0 2004-03
 *  3GPP AMR Floating-point Speech Codec
 * ===================================================================
 *
 */

/*
 * interf_enc.c
 *
 *
 * Project:
 *    AMR Floating-Point Codec
 *
 * Contains:
 *    This module contains all the functions needed encoding 160
 *    16-bit speech samples to AMR encoder parameters.
 *
 */

/*
 * include files
 */
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include "sp_enc.h"
#include "interf_rom.h"

/*
 * Declare structure types
 */
/* Declaration transmitted frame types */
enum TXFrameType { TX_SPEECH_GOOD = 0,
                   TX_SID_FIRST,
                   TX_SID_UPDATE,
                   TX_NO_DATA,
                   TX_SPEECH_DEGRADED,
                   TX_SPEECH_BAD,
                   TX_SID_BAD,
                   TX_ONSET,
                   TX_N_FRAMETYPES     /* number of frame types */
};

/* Declaration of interface structure */
typedef struct
{
   Word16 sid_update_counter;   /* Number of frames since last SID */
   Word16 sid_handover_debt;   /* Number of extra SID_UPD frames to schedule */
   Word32 dtx;
   enum TXFrameType prev_ft;   /* Type of the previous frame */
   void *encoderState;   /* Points encoder state structure */
} enc_interface_State;


#ifdef ETSI
/*
 * Prm2Bits
 *
 *
 * Parameters:
 *    value             I: value to be converted to binary
 *    no_of_bits        I: number of bits associated with value
 *    bitstream         O: address where bits are written
 *
 * Function:
 *    Convert integer to binary and write the bits to the array.
 *    The most significant bits are written first.
 * Returns:
 *    void
 */
static void Int2Bin( Word16 value, Word16 no_of_bits, Word16 *bitstream )
{
   Word32 i, bit;
   Word16 *pt_bitstream;

   pt_bitstream = &bitstream[no_of_bits];

   for ( i = 0; i < no_of_bits; i++ ) {
      bit = value & 0x0001;

      if ( bit == 0 ) {
         * --pt_bitstream = 0;
      }
      else {
         * --pt_bitstream = 1;
      }
      value = ( Word16 )( value >> 1 );
   }
}


/*
 * Prm2Bits
 *
 *
 * Parameters:
 *    mode              I: AMR mode
 *    prm               I: analysis parameters
 *    bits              O: serial bits
 *
 * Function:
 *    converts the encoder parameter vector into a vector of serial bits.
 * Returns:
 *    void
 */
static void Prm2Bits( enum Mode mode, Word16 prm[], Word16 bits[] )
{
   Word32 i;

   switch ( mode ) {
      case MR122:
         for ( i = 0; i < PRMNO_MR122; i++ ) {
            Int2Bin( prm[i], bitno_MR122[i], bits );
            bits += bitno_MR122[i];
         }
         break;

      case MR102:
         for ( i = 0; i < PRMNO_MR102; i++ ) {
            Int2Bin( prm[i], bitno_MR102[i], bits );
            bits += bitno_MR102[i];
         }
         break;

      case MR795:
         for ( i = 0; i < PRMNO_MR795; i++ ) {
            Int2Bin( prm[i], bitno_MR795[i], bits );
            bits += bitno_MR795[i];
         }
         break;

      case MR74:
         for ( i = 0; i < PRMNO_MR74; i++ ) {
            Int2Bin( prm[i], bitno_MR74[i], bits );
            bits += bitno_MR74[i];
         }
         break;

      case MR67:
         for ( i = 0; i < PRMNO_MR67; i++ ) {
            Int2Bin( prm[i], bitno_MR67[i], bits );
            bits += bitno_MR67[i];
         }
         break;

      case MR59:
         for ( i = 0; i < PRMNO_MR59; i++ ) {
            Int2Bin( prm[i], bitno_MR59[i], bits );
            bits += bitno_MR59[i];
         }
         break;

      case MR515:
         for ( i = 0; i < PRMNO_MR515; i++ ) {
            Int2Bin( prm[i], bitno_MR515[i], bits );
            bits += bitno_MR515[i];
         }
         break;

      case MR475:
         for ( i = 0; i < PRMNO_MR475; i++ ) {
            Int2Bin( prm[i], bitno_MR475[i], bits );
            bits += bitno_MR475[i];
         }
         break;

      case MRDTX:
         for ( i = 0; i < PRMNO_MRDTX; i++ ) {
            Int2Bin( prm[i], bitno_MRDTX[i], bits );
            bits += bitno_MRDTX[i];
         }
         break;
   }
   return;
}

#else

#ifndef IF2

/*
 * EncoderMMS
 *
 *
 * Parameters:
 *    mode                 I: AMR mode
 *    param                I: Encoder output parameters
 *    stream               O: packed speech frame
 *    frame_type           I: frame type (DTX)
 *    speech_mode          I: speech mode (DTX)
 *
 * Function:
 *    Pack encoder output parameters to octet structure according
 *    importance table and AMR file storage format according to
 *    RFC 3267.
 * Returns:
 *    number of octets
 */
static int EncoderMMS( enum Mode mode, Word16 *param, UWord8 *stream, enum
      TXFrameType frame_type, enum Mode speech_mode )
{
   Word32 j = 0, k;
   Word16 *mask;

   memset(stream, 0, block_size[mode]);

   *stream = toc_byte[mode];
   stream++;

   if ( mode == 15 ) {
      return 1;
   }
   else if ( mode == MRDTX ) {
      mask = order_MRDTX;

      for ( j = 1; j < 36; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x01;
         mask += 2;

         if ( j % 8 )
            *stream <<= 1;
         else
            stream++;
      }

      /* add SID type information */
      if ( frame_type == TX_SID_UPDATE )
         *stream += 0x01;
      *stream <<= 3;

      /* speech mode indication */
      *stream += ( unsigned char )(((speech_mode & 0x0001) << 2) | (speech_mode & 0x0002) | ((speech_mode & 0x0004) >> 2));

	  *stream <<= 1;

      /* don't shift at the end of the function */
      return 6;
   }
   else if ( mode == MR475 ) {
      mask = order_MR475;

      for ( j = 1; j < 96; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x01;
         mask += 2;

         if ( j % 8 )
            *stream <<= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR515 ) {
      mask = order_MR515;

      for ( j = 1; j < 104; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x01;
         mask += 2;

         if ( j % 8 )
            *stream <<= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR59 ) {
      mask = order_MR59;

      for ( j = 1; j < 119; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x01;
         mask += 2;

         if ( j % 8 )
            *stream <<= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR67 ) {
      mask = order_MR67;

      for ( j = 1; j < 135; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x01;
         mask += 2;

         if ( j % 8 )
            *stream <<= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR74 ) {
      mask = order_MR74;

      for ( j = 1; j < 149; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x01;
         mask += 2;

         if ( j % 8 )
            *stream <<= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR795 ) {
      mask = order_MR795;

      for ( j = 1; j < 160; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x01;
         mask += 2;

         if ( j % 8 )
            *stream <<= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR102 ) {
      mask = order_MR102;

      for ( j = 1; j < 205; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x01;
         mask += 2;

         if ( j % 8 )
            *stream <<= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR122 ) {
      mask = order_MR122;

      for ( j = 1; j < 245; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x01;
         mask += 2;

         if ( j % 8 )
            *stream <<= 1;
         else
            stream++;
      }
   }

   /* shift remaining bits */
   if ( (k = j % 8) )	*stream <<= ( 8 - k );
   return( (int)block_size[mode] );
}

#else

/*
 * Encoder3GPP
 *
 *
 * Parameters:
 *    mode                 I: AMR mode
 *    param                I: Encoder output parameters
 *    stream               O: packed speech frame
 *    frame_type           I: frame type (DTX)
 *    speech_mode          I: speech mode (DTX)
 *
 * Function:
 *    Pack encoder output parameters to octet structure according
 *    importance table.
 * Returns:
 *    number of octets
 */
static int Encoder3GPP( enum Mode mode, Word16 *param, UWord8 *stream, enum
      TXFrameType frame_type, enum Mode speech_mode )
{
   Word32 j = 0;
   Word16 *mask;

   memset(stream, 0, block_size[mode]);

   if ( mode == 15 ) {
      *stream = 0xF;
      return 1;
   }
   else if ( mode == MRDTX ) {
      mask = order_MRDTX;
      *stream = 0x40;

      for ( j = 5; j < 40; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x80;
         mask += 2;

         if ( j % 8 )
            *stream >>= 1;
         else
            stream++;
      }

      /* add SID type information */
      if ( frame_type == TX_SID_UPDATE )
         *stream += 0x80;
      stream++;

      /* speech mode indication */
      *stream = ( unsigned char )speech_mode;

      /* don't shift at the end of the function */
      return 6;
   }
   else if ( mode == MR475 ) {
      mask = order_MR475;
      *stream = 0;

      for ( j = 5; j < 100; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x80;
         mask += 2;

         if ( j % 8 )
            *stream >>= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR515 ) {
      mask = order_MR515;
      *stream = 0x8;

      for ( j = 5; j < 108; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x80;
         mask += 2;

         if ( j % 8 )
            *stream >>= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR59 ) {
      mask = order_MR59;
      *stream = 0x10;

      for ( j = 5; j < 123; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x80;
         mask += 2;

         if ( j % 8 )
            *stream >>= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR67 ) {
      mask = order_MR67;
      *stream = 0x18;

      for ( j = 5; j < 139; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x80;
         mask += 2;

         if ( j % 8 )
            *stream >>= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR74 ) {
      mask = order_MR74;
      *stream = 0x20;

      for ( j = 5; j < 153; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x80;
         mask += 2;

         if ( j % 8 )
            *stream >>= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR795 ) {
      mask = order_MR795;
      *stream = 0x28;

      for ( j = 5; j < 164; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x80;
         mask += 2;

         if ( j % 8 )
            *stream >>= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR102 ) {
      mask = order_MR102;
      *stream = 0x30;

      for ( j = 5; j < 209; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x80;
         mask += 2;

         if ( j % 8 )
            *stream >>= 1;
         else
            stream++;
      }
   }
   else if ( mode == MR122 ) {
      mask = order_MR122;
      *stream = 0x38;

      for ( j = 5; j < 249; j++ ) {
         if ( param[ * mask] & *( mask + 1 ) )
            *stream += 0x80;
         mask += 2;

         if ( j % 8 )
            *stream >>= 1;
         else
            stream++;
      }
   }

   /* shift remaining bits */
   *stream >>= ( 8 - j % 8 );
   return( (int)block_size[mode] );
}
#endif
#endif

/*
 * Sid_Sync_reset
 *
 *
 * Parameters:
 *    st                O: state structure
 *
 * Function:
 *    Initializes state memory
 *
 * Returns:
 *    void
 */
static void Sid_Sync_reset( enc_interface_State *st )
{
   st->sid_update_counter = 3;
   st->sid_handover_debt = 0;
   st->prev_ft = TX_SPEECH_GOOD;
}


/*
 * Encoder_Interface_Encode
 *
 *
 * Parameters:
 *    st                I: pointer to state structure
 *    mode              I: Speech Mode
 *    speech            I: Input speech
 *    serial            O: Output octet structure 3GPP or
 *                         ETSI serial stream
 *    force_speech      I: Force speech in DTX
 *
 * Function:
 *    Encoding and packing one frame of speech
 *
 * Returns:
 *    number of octets
 */
int Encoder_Interface_Encode( void *st, enum Mode mode, Word16 *speech,

#ifndef ETSI
      UWord8 *serial,

#else
      Word16 *serial,
#endif

      int force_speech )
{
   Word16 prm[PRMNO_MR122];   /* speech parameters, max size */
   const Word16 *homing;   /* pointer to homing frame */
   Word16 homing_size;   /* frame size for homing frame */


   enc_interface_State * s;
   enum TXFrameType txFrameType;   /* frame type */

   int i, noHoming = 0;


   /*
    * used encoder mode,
    * if used_mode == -1, force VAD on
    */
   enum Mode used_mode = -force_speech;


   s = ( enc_interface_State * )st;

    /*
     * Checks if all samples of the input frame matches the encoder
     * homing frame pattern, which is 0x0008 for all samples.
     */
   for ( i = 0; i < 160; i++ ) {
      noHoming = speech[i] ^ 0x0008;

      if ( noHoming )
         break;
   }

   if (noHoming){
      Speech_Encode_Frame( s->encoderState, mode, speech, prm, &used_mode );
   }
   else {
      switch ( mode ) {
         case MR122:
            homing = dhf_MR122;
            homing_size = 18;
            break;

         case MR102:
            homing = dhf_MR102;
            homing_size = 12;
            break;

         case MR795:
            homing = dhf_MR795;
            homing_size = 8;
            break;

         case MR74:
            homing = dhf_MR74;
            homing_size = 7;
            break;

         case MR67:
            homing = dhf_MR67;
            homing_size = 7;
            break;

         case MR59:
            homing = dhf_MR59;
            homing_size = 7;
            break;

         case MR515:
            homing = dhf_MR515;
            homing_size = 7;
            break;

         case MR475:
            homing = dhf_MR475;
            homing_size = 7;
            break;

         default:
            homing = NULL;
            homing_size = 0;
            break;
      }
      for( i = 0; i < homing_size; i++){
         prm[i] = homing[i];
      }
      /* rest of the parameters are zero */
      memset(&prm[homing_size], 0, (PRMNO_MR122 - homing_size) << 1);
      used_mode = mode;
   }
   if ( used_mode == MRDTX ) {
      s->sid_update_counter--;

      if ( s->prev_ft == TX_SPEECH_GOOD ) {
         txFrameType = TX_SID_FIRST;
         s->sid_update_counter = 3;
      }
      else {
         /* TX_SID_UPDATE or TX_NO_DATA */
         if ( ( s->sid_handover_debt > 0 ) && ( s->sid_update_counter > 2 ) ) {
              /*
               * ensure extra updates are properly delayed after
               * a possible SID_FIRST
               */
            txFrameType = TX_SID_UPDATE;
            s->sid_handover_debt--;
         }
         else {
            if ( s->sid_update_counter == 0 ) {
               txFrameType = TX_SID_UPDATE;
               s->sid_update_counter = 8;
            }
            else {
               txFrameType = TX_NO_DATA;
               used_mode = 15;
            }
         }
      }
   }
   else {
      s->sid_update_counter = 8;
      txFrameType = TX_SPEECH_GOOD;
   }
   s->prev_ft = txFrameType;

   if ( noHoming == 0 ) {
      Speech_Encode_Frame_reset( s->encoderState, s->dtx );
      Sid_Sync_reset( s );
   }

#ifndef ETSI
#ifdef IF2
   return Encoder3GPP( used_mode, prm, serial, txFrameType, mode );

#else
   return EncoderMMS( used_mode, prm, serial, txFrameType, mode );

#endif
#else

   Prm2Bits( used_mode, prm, &serial[1] );
   serial[0] = ( Word16 )txFrameType;
   serial[245] = ( Word16 )mode;
   return 500;
#endif

}


/*
 * Encoder_Interface_init
 *
 *
 * Parameters:
 *    dtx               I: DTX flag
 *
 * Function:
 *    Allocates state memory and initializes state memory
 *
 * Returns:
 *    pointer to encoder interface structure
 */
void * Encoder_Interface_init( int dtx )
{
   enc_interface_State * s;

   /* allocate memory */
   if ( ( s = ( enc_interface_State * ) malloc( sizeof( enc_interface_State ) ) ) ==
         NULL ) {
      fprintf( stderr, "Encoder_Interface_init: "
            "can not malloc state structure\n" );
      return NULL;
   }
   s->encoderState = Speech_Encode_Frame_init( dtx );
   Sid_Sync_reset( s );
   s->dtx = dtx;
   return s;
}


/*
 * DecoderInterfaceExit
 *
 *
 * Parameters:
 *    state             I: state structure
 *
 * Function:
 *    The memory used for state memory is freed
 *
 * Returns:
 *    Void
 */
void Encoder_Interface_exit( void *state )
{
   enc_interface_State * s;
   s = ( enc_interface_State * )state;

   /* free memory */
   Speech_Encode_Frame_exit( &s->encoderState );
   free( s );
   state = NULL;
}


//encode using 475-mode and pack parameter and DTX-flag to 12 bytes (96 bits)
//return data length in bytes: 12 for speech, 5 for SID
//set force_speech=1 for VAD
int AMR475_encode( void *st, Word16 *speech, UWord8 *serial, int force_speech )
{
 Word16 prm[PRMNO_MR475]; //parameters outputed by encoder
 enc_interface_State * s; //pointer to state
 int i, j, noHoming = 0;	  //homing detect flag
 Word16 bits;			  //output bits counter

  /*
    * used encoder mode,
    * if used_mode == -1, force VAD on
    */
   enum Mode used_mode = -force_speech;


   s = ( enc_interface_State * )st;

     /*
     * Checks if all samples of the input frame matches the encoder
     * homing frame pattern, which is 0x0008 for all samples.
     */
   for ( i = 0; i < 160; i++ ) {
      noHoming = speech[i] ^ 0x0008;

      if ( noHoming )
         break;
   }

//process speech frame 
   if (noHoming) Speech_Encode_Frame( s->encoderState, MR475, speech, prm, &used_mode );
   else //process homing
   {
    for( i = 0; i < 7; i++) prm[i] = dhf_MR475[i]; //set homing for decoder
     /* rest of the parameters are zero */
    memset(&prm[7], 0, (PRMNO_MR475 - 7) << 1);
    used_mode = MR475; //mark as normal frame
    Speech_Encode_Frame_reset( s->encoderState, s->dtx ); //reset encoder
    Sid_Sync_reset( s ); //rresed DTX
   }
    
//pack parameters to bits
   memset(serial, 0, 12);
   
   if(used_mode == MRDTX) //pack SID to 5 bits 
   {
    bits=5; //reserve 5 bits for SID flag and DTX counter
    for ( i = 0; i < PRMNO_MRDTX; i++ ) //process each parameter
    {
     for ( j = 0; j < bitno_MRDTX[i]; j++ ) //process eack bits in it
     { //set corresponding bit in byte
      if(prm[i] & ((Word16)1<<j)) serial[bits>>3]|=((UWord8)1<<(bits & 0x07));
      bits++; //updates bits counter
     }
    }
    serial[0] |= 0x01; //set dtx flag   
    return 5;
   }
   else 
   {
    bits=1; //reserve first bit for DTX flag
    for ( i = 0; i < PRMNO_MR475; i++ ) //pack speech to 12 bits
    {
     for ( j = 0; j < bitno_MR475[i]; j++ ) 
     {
      if(prm[i] & ((Word16)1<<j)) serial[bits>>3]|=((UWord8)1<<(bits & 0x07));
      bits++; //updates bits counter
     }
    }
    return 12;
   }
}


//encode using mode, pack parameter and SID-flag=0 to bytes
//return data length in bytes (5 for SID data for all modes)
//set force_speech=1 for cbr (no VAD function)
int AMR_encode( void *st, UWord8 mode, Word16 *speech, UWord8 *serial, int force_speech )
{
 
 Word16 prm[PRMNO_MR122]; //parameters outputed by encoder
 enc_interface_State * s; //pointer to state
 int i, j, noHoming = 0;	  //homing detect flag
 Word16 bits;			  //output bits counter
 
 /*  Modes list
 Mode 0: 95 (12)
 Mode 1: 103 (13)
 Mode 2: 118 (15)
 Mode 3: 134 (17)
 Mode 4: 148 (19)
 Mode 5: 159 (20)
 Mode 6: 204 (26)
 Mode 7: 244 (31)
 */ 
 
 //Homing frames length in bytes for each mode
 UWord8 homing_size_m[8]={7,7,7,7,7,8,12,18};
 
 //Pointers to homing frames patterns for each mode
 const Word16 *dhf_m[8]={dhf_MR475, dhf_MR515, dhf_MR59,
 	dhf_MR67, dhf_MR74, dhf_MR795, dhf_MR102, dhf_MR122};

 //Number of parameters for each mode
 UWord8 prmno_m[8]={PRMNO_MR475, PRMNO_MR515, PRMNO_MR59,
 	PRMNO_MR67, PRMNO_MR74, PRMNO_MR795, PRMNO_MR102, PRMNO_MR122};
 
 //Number of bits in parameters for each mode
 const Word16 *bitno_m[8]={bitno_MR475, bitno_MR515, bitno_MR59,
 	bitno_MR67, bitno_MR74, bitno_MR795, bitno_MR102, bitno_MR122};
 
 //Encodec blocks length in byte for each mode	
 UWord8 bl_size_m[8]={12,13,15,17,19,20,26,31};
 
//Set variables for current mode   
 UWord8 homing_size=homing_size_m[mode];
 const Word16 *dhf=dhf_m[mode];
 UWord8 prmno=prmno_m[mode];
 const Word16 *bitno=bitno_m[mode];
 UWord8 bl_size=bl_size_m[mode];


  /*
    * used encoder mode,
    * if used_mode == -1, force VAD on
    */
   enum Mode used_mode = -force_speech;
   

  //encoder's state 
   s = ( enc_interface_State * )st;

     /*
     * Checks if all samples of the input frame matches the encoder
     * homing frame pattern, which is 0x0008 for all samples.
     */
   for ( i = 0; i < 160; i++ ) {
      noHoming = speech[i] ^ 0x0008;

      if ( noHoming )
         break;
   }

//process speech frame: used_mode returns type: MRDTX for SID or current mode for speech 
   if (noHoming) Speech_Encode_Frame( s->encoderState, (enum Mode)mode, speech, prm, &used_mode );
   else //process homing
   {
    for( i = 0; i < homing_size; i++) prm[i] = dhf[i]; //set homing for decoder
    memset(&prm[homing_size], 0, (prmno - homing_size) << 1); //rest of the parameters are zero
    used_mode = (enum Mode)mode; //mark as normal frame
    Speech_Encode_Frame_reset( s->encoderState, s->dtx ); //reset encoder
    Sid_Sync_reset( s ); //reset DTX
   }
    
//pack parameters to bits
   memset(serial, 0, bl_size); //clear memory for bits block
   
   if(used_mode == MRDTX) //pack SID to 5 bits 
   {
    bits=5; //reserve 5 bits for SID flag=1 (bit 0) and DTX counter
    for ( i = 0; i < PRMNO_MRDTX; i++ ) //process each parameter
    {
     for ( j = 0; j < bitno_MRDTX[i]; j++ ) //process eack bits in it
     { //set corresponding bit in byte
      if(prm[i] & ((Word16)1<<j)) serial[bits>>3]|=((UWord8)1<<(bits & 0x07));
      bits++; //updates bits counter
     }
    }
    serial[0] |= 0x01; //set dtx flag   
    return 5; //fixed length of SID block for all modes
   }
   else //this was a speech frame
   {
    bits=1; //reserve first bit for no-SID flag=0 (bit 0)
    for ( i = 0; i < prmno; i++ ) //pack speech to bits
    {
     for ( j = 0; j < bitno[i]; j++ ) 
     {
      if(prm[i] & ((Word16)1<<j)) serial[bits>>3]|=((UWord8)1<<(bits & 0x07));
      bits++; //updates bits counter
     }
    }
    return bl_size; //size of encoded block for current mode
   }
}


