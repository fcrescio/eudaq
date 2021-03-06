#include <ostream>
#include <iostream>
#include <time.h>

#include "eudaq/BufferSerializer.hh"
#include "eudaq/AidaPacket.hh"

using std::cout;

namespace eudaq {
	EUDAQ_DEFINE_PACKET(EventPacket, str2type( "-EVWRAP-") );


	void AidaPacket::SerializeHeader( Serializer& s ) const {
		uint64_t* arr = (uint64_t *)&m_header;
		for ( int i = 0; i < sizeof( m_header ) / sizeof( uint64_t ); i++ )
			s.write( arr[i] );
	}

	void AidaPacket::SerializeMetaData( Serializer& s ) const {
		  s.write( m_meta_data.v );
	}

	AidaPacket::PacketHeader AidaPacket::DeserializeHeader( Deserializer& ds ) {
		PacketHeader header;
		uint64_t* arr = (uint64_t *)&header;
		for ( int i = 0; i < sizeof( header ) / sizeof( uint64_t ); i++ )
			ds.read( arr[i] );
		return header;
	}

    const uint64_t * const AidaPacket::bit_mask() {
    	static uint64_t* array = NULL;
    	if ( !array ) {
    		array = new uint64_t[65];
    		array[0] = 0;
    		array[1] = 1;
    		for ( int i = 2; i <= 64; i++ ) {
    			array[i] = (array[i - 1] << 1) + 1;
    		}
    	}
    	return array;
    }


  uint64_t AidaPacket::str2type(const std::string & str) {
    uint64_t result = 0;
    for (size_t i = str.length(); i > 0; i-- ) {
    	result <<= 8;
    	result |= str[ i - 1 ];
    }
    cout << "str2type: str=>" << str << "< uint64=" << std::hex << result << std::endl;
    return result;
  }

  std::string AidaPacket::type2str(uint64_t id) {
    //std::cout << "id2str(" << std::hex << id << std::dec << ")" << std::flush;
    std::string result(8, '\0');
    for (int i = 0; i < 8; ++i) {
      result[i] = (char)(id & 0xff);
      id >>= 8;
    }
    for (int i = 7; i >= 0; --i) {
      if (result[i] == '\0') {
        result.erase(i);
        break;
      }
    }
    //std::cout << " = " << result << std::endl;
    return result;
  }

  void AidaPacket::Print(std::ostream & os) const {
    os << "Type=" << type2str( GetPacketType() )
      << ", packet#=" << GetPacketNumber();
  }

  std::ostream & operator << (std::ostream &os, const AidaPacket &packet) {
    packet.Print(os);
    return os;
  }




  EventPacket::EventPacket( const Event & ev ) : m_ev( &ev ) {
    	m_header.packetType    = get_type();
    	m_header.packetSubType = 0;
    	m_header.packetNumber  = m_ev->GetEventNumber();

    	m_meta_data.add( false, MetaDataType::RUN_NUMBER, m_ev->GetRunNumber() );
    	m_meta_data.add( false, MetaDataType::TRIGGER_COUNTER, m_ev->GetEventNumber() );
    	m_meta_data.add( false, MetaDataType::TRIGGER_TIMESTAMP, m_ev->GetTimestamp() );
  }


  EventPacket::EventPacket( PacketHeader& header, Deserializer & ds) {
		m_header = header;
//		ds.read( m_meta_data );
		m_ev = EventFactory::Create( ds );
		ds.read( checksum );
  }


  void EventPacket::Serialize(Serializer & ser) const {
	  // std::cout << "Serialize ev# = " << std::hex << GetPacketNumber() << std::endl;
	  SerializeHeader( ser );
	  SerializeMetaData( ser );
	  m_ev->Serialize( ser );
	  ser.write( ser.GetCheckSum() );
  }




  AidaPacket * PacketFactory::Create( Deserializer & ds) {
	  AidaPacket::PacketHeader header = AidaPacket::DeserializeHeader( ds );
	  int id = header.packetType;
      //std::cout << "Create id = " << std::hex << id << std::dec << std::endl;
      packet_creator cr = GetCreator(id);
      if (!cr) EUDAQ_THROW("Unrecognised packet type (" + to_string(id) + ")");
      return cr( header, ds);
  };

  PacketFactory::map_t & PacketFactory::get_map() {
    static map_t s_map;
    return s_map;
  }

  void PacketFactory::Register( uint64_t id, PacketFactory::packet_creator func) {
    // TODO: check id is not already in map
    get_map()[id] = func;
  }

  PacketFactory::packet_creator PacketFactory::GetCreator(int id) {
    // TODO: check it exists...
    return get_map()[id];
  }

}
