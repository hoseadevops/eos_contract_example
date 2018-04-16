/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @auther: hanfei
 *  @createtime: 2018-04-16
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/contract.hpp>

using eosio::indexed_by;
using eosio::const_mem_fun;
using std::string;


class blog_view : public eosio::contract {
    public:
        using contract::contract;
        blog_view(account_name self)
            :eosio::contract(self),
            accounts(_self, _self),
            idlists(_self, _self),
            init_status(std::string(64,'0'))
            {}

        /// @abi action
        void upload(const account_name producer, const std::string content) {
            require_auth(producer);
            
            blog_index upload_blogs(_self, producer);
            //获取ID
            uint32_t nowID = get_ID();

            //TODO: add the dedup
            upload_blogs.emplace(producer, [&]( auto& g ) {
                g.ID = nowID;
                g.status = Status::s_uploaded;
                g.producer = producer;
                g.content = content;
                g.approve_status = init_status;
            });

            //在idlist添加，ID--producer关系，用于之后通过ID查询producer
            idlists.emplace(_self, [&]( auto& g ) {
                g.ID = nowID;
                g.producer = producer;
            });

            //blog数量+1
            blognum_op(producer, '+');
        }

        /// @abi action
        void reviewing(const uint64_t ID, const account_name reviewer) {
            require_auth(reviewer);

            //先通过idlist查询ID，获取用户名，然后才能用mutil_index查询具体用户的blog，下同
            auto itrid = idlists.find(ID);
            eosio_assert(itrid != idlists.end(), "this blog doesn't exists!\n");

            blog_index review_blogs(_self, itrid->producer);

            auto itr = review_blogs.find( ID );
            eosio_assert(itr != review_blogs.end(), "this blog doesn't exists!\n");
            eosio_assert(itr->producer != reviewer, "you can't review youself!\n");
            eosio_assert(itr->status == Status::s_uploaded, "this blog is reviewing or reviewed!\n");
            
            review_blogs.modify(itr, itrid->producer, [&](auto& g){
                g.status = Status::s_reviewing;
                g.reviewer = reviewer;
            });
        }

        /// @abi action
        void approved(const uint64_t ID) {

            auto itrid = idlists.find(ID);
            eosio_assert(itrid != idlists.end(), "this blog doesn't exists!\n");
            blog_index approve_blogs(_self, itrid->producer);

            auto itr = approve_blogs.find( ID );
            eosio_assert(itr != approve_blogs.end(), "this blog doesn't exists!\n");
            eosio_assert(itr->status == Status::s_reviewing, "this blog is reviewed!\n");
            require_auth(itr->reviewer);
            
            approve_blogs.modify(itr, itrid->producer, [&](auto& g){
                g.status = Status::s_approved;
                g.approve_status = std::string("approved");
            });
        }

        /// @abi action
        void disapprove(const uint64_t ID, std::string reason) {

            auto itrid = idlists.find(ID);
            eosio_assert(itrid != idlists.end(), "this blog doesn't exists!\n");
            blog_index disappr_policys(_self, itrid->producer);

            auto itr = disappr_policys.find( ID );
            eosio_assert(itr != disappr_policys.end(), "this blog doesn't exists!\n");
            eosio_assert(itr->status == Status::s_reviewing, "this blog is reviewed!\n");
            require_auth(itr->reviewer);
            
            disappr_policys.modify(itr, itrid->producer, [&](auto& g){
                g.status = Status::s_disapprove;
                g.approve_status = reason;
            });
        }

        /// @abi action
        void remove(const uint64_t ID) {
            auto itrid = idlists.find(ID);
            eosio_assert(itrid != idlists.end(), "this blog doesn't exists!\n");
            blog_index remove_policys(_self, itrid->producer);

            auto itr = remove_policys.find( ID );
            eosio_assert(itr != remove_policys.end(), "this blog doesn't exists!\n");
            require_auth(itr->producer);
            
            remove_policys.erase(itr);
            blognum_op(itr->producer, '-');
        }


    private:
        enum Status {s_uploaded,s_reviewing,s_approved,s_disapprove};

        /**
         * 用于保存用户信息，保存用户文章数量
         * @abi table account i64
         */
        struct account {
            account( account_name o = account_name() ):owner(o){}

            account_name owner;
            uint32_t     blognum = 0;

            bool is_empty()const { return !blognum; }

            uint64_t primary_key()const { return owner; }

            EOSLIB_SERIALIZE( account, (owner)(blognum) )
        };

        typedef eosio::multi_index< N(account), account> account_index;

        /**
         * 保存文章ID和用户关系，假如不保存，则审核员每次都要提交文章作者；
         */
        struct idlist {
            uint64_t ID;
            account_name producer;

            uint64_t primary_key()const { return ID; }

            EOSLIB_SERIALIZE( idlist, (ID)(producer) )
        };
        typedef eosio::multi_index< N(idlist), idlist> idlist_index;

        //@abi table blog i64
        struct blog {
            uint64_t ID;
            uint8_t status;
            std::string approve_status;
            account_name producer;
            account_name reviewer;
            std::string content;

            auto primary_key() const { return ID; }

            EOSLIB_SERIALIZE( blog, (ID)(status)(approve_status)(producer)(reviewer)(content) )
        }; 

        typedef eosio::multi_index< N(blog), blog> blog_index;

        account_index accounts;
        idlist_index idlists;
        std::string init_status;

        // get the code's policynum
        uint32_t get_ID() {
            auto itr = accounts.find( _self );
            if ( itr == accounts.end() ) {
                return 0;
            } else {
                return itr->blognum;
            }
        }

        /**
         * to operate the account's policynum
         * op : '+','-'
         */
        void blognum_op(account_name name, char op) {
            auto itr = accounts.find( name );
            if ( itr == accounts.end() ) {
                accounts.emplace(name, [&]( auto& g ) {
                    g.owner = name;
                    g.blognum = 1;
                });
            } else {
                if( op == '+' ) {
                    accounts.modify(itr, itr->owner, []( auto& g ) {g.blognum += 1;});
                } else if( op == '-' ) {
                    accounts.modify(itr, itr->owner, []( auto& g ) {g.blognum -= 1;});
                }
            }

            itr = accounts.find( _self );
            if ( itr == accounts.end() ) {
                accounts.emplace(_self, [&]( auto& g ) {
                    g.owner = _self;
                    g.blognum = 1;
                });
            } else {
                if( op == '+' ) {
                    accounts.modify(itr, itr->owner, []( auto& g ) {g.blognum += 1;});
                }
            }
        }
};

EOSIO_ABI( blog_view, (upload)(reviewing)(approved)(disapprove)(remove) )
